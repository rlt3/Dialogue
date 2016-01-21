#include <stdlib.h>
#include <pthread.h>
#include "company.h"

/*
 * TODO:
 * The Company is a tree of Actors. To keep tree operations thread-safe and to 
 * shoot for as little of lock-time possible, we have a specific setup.
 *
 * Individual nodes of the tree are kept in an array. The array index is that
 * node's id (and consequently, the Actor's too, who is occupying that node). 
 * The Nodes *are* part of the tree, but can be free from the tree too. So,
 * each Node has a state of attached or detached.
 *
 * We keep a count of the total number of actors under a mutex. Each node is
 * protected by a read/write lock. The read/write lock keeps throughput (reads)
 * of the structure flowing (reads from multiple threads) until it changes. And
 * since each Node has its own read/write lock, we can have parts of the tree
 * readable while others are locked.
 *
 * The node points (by using ids) to three other Nodes, the parent, its next
 * sibling, and its first child. This way, all references within the tree are
 * just one step away and isolated.
 *
 * If we remove any node, its reference can be updated throughout the tree as
 * the errors occur. So, when a `next_sibling' reference is pulled up, we can
 * automate it be set to -1 (our code for nil) when its references inevitably
 * returns as bad (which it will if a Node exists, yet is not attached).
 *
 * The system automatically filters itself of bad references and errors out
 * gracefully for anything else that uses bad references too.
 *
 * (To arbitrarily remove a Node, I need to lock both the Actor mutex and the
 * write lock on the Node itself. While references can be handled gracefully,
 * we cannot free memory which another thread might be currently reading.)
 */

typedef struct Node {
    Actor *actor;
    int attached;
    /* these are indices to other Nodes */
    int parent;
    int next_sibling;
    int first_child;
    pthread_rwlock_t rw_lock;
} Node;

struct Company {
    Node *list;
    int list_size;
    int nodes_in_use;
    pthread_rwlock_t rw_lock;
};

/*
 * Make sure the id is a valid index so we aren't accessing memory out of 
 * bounds. If id is less than 0, we can assume an invalid index. Otherwise, we
 * have to do a read lock on the list's size.
 */
int
node_index_valid (Company *company, int id)
{
    int ret = 0;

    if (id < 0)
        goto exit;

    pthread_rwlock_rdlock(&company->rw_lock);
    ret = !(id >= company->list_size);
    pthread_rwlock_unlock(&company->rw_lock);

    return ret;
}

/*
 * Returns 0 and *does not* acquire the lock if id is not a valid index.
 * Acquires the write lock otherwise and returns 1.
 */
inline int
node_write (Company *company, int id)
{
    if (!node_valid_index(company, id))
        return 0;
    pthread_rwlock_wrlock(&company->list[id].rw_lock);
    return 1;
}

/*
 * Returns 0 and *does not* acquire the lock if id is not a valid index.
 * Acquires the read lock otherwise and returns 1.
 */
inline int
node_read (Company *company, int id)
{
    if (!node_valid_index(company, id))
        return 0;
    pthread_rwlock_rdlock(&company->list[id].rw_lock);
    return 1;
}

/*
 * *Does not* do anything if id isn't a valid index. Unlocks the read/write
 * lock otherwise.
 */
inline void
node_unlock (Company *company, int id)
{
    if (!node_valid_index(company, id))
        return;
    pthread_rwlock_unlock(&company->list[id].rw_lock);
}

/*
 * Get the family member of the Node at the given id. If the id isn't valid,
 * this returns -1. Looks for the family member by type. If the type isn't 
 * valid, this returns -1. If the family member of the Node isn't valid it is
 * replaced with -1 and -1 is returned, otherwise the valid family member
 * reference is returned.
 */
int
node_get_family (Company *company, int id, int which)
{
    int ret = -1;
    int *ptr = NULL;

    if (!node_read(company, id))
        goto unlock;
            
    /* Get the pointer to which family we're verifying. */
    switch (which) {
    case 0:
        ptr = &company->list[id].parent;
        break;

    case 1:
        ptr = &company->list[id].next_sibling;
        break;

    case 2:
        ptr = &company->list[id].first_child;
        break;

    default: /* Not a valid `which` */
        goto unlock;
        break;
    }

    /* if the read is successful (*member is valid) */
    if (node_read(company, *ptr)) {
        /* If the Node isn't attached and has no Actor, it isn't valid */
        if (!company->list[*ptr].attached && company->list[*ptr] == NULL) {
            *ptr = -1;
        } else {
            ret = *ptr;
        }
    }
    node_lock(company, *ptr);

unlock:
    node_lock(company, id);
    return ret;
}

/*
 * Setup a node (an element of the Company's list).
 */
void
node_init (Company *company, int id, Actor *actor)
{
    if (!node_write(company, id))
        return;
    company->list[id].actor = actor;
    company->list[id].attached = 1;
    company->list[id].parent = -1;
    company->list[id].next_sibling = -1;
    company->list[id].first_child = -1;
    pthread_rwlock_init(&company->list[id].rw_lock, NULL);
    node_unlock(company, id);
}

/*
 * Cleanup a node, destroying the Actor and setting its pointer to NULL,
 * set references to zero, and destroying the ref mutex and family rwlock.
 */
void
node_cleanup (Company *company, int id)
{
    if (!node_write(company, id))
        return;
    actor_destroy(company->list[id].actor);
    company->list[id].actor = NULL;
    company->list[id].attached = 0;
    company->list[id].parent = -1;
    company->list[id].next_sibling = -1;
    company->list[id].first_child = -1;
    pthread_rwlock_destroy(&company->list[id].rw_lock);
    node_unlock(company, id);
}

Company *
company_create (int buffer_length)
{
    int i;
    Company *company = malloc(sizeof(*company));

    if (company == NULL)
        goto exit;

    company->list = malloc(sizeof(Node) * buffer_length);

    if (company->list == NULL) {
        free(company);
        company = NULL;
        goto exit;
    }

    company->list_size = buffer_length;
    company->nodes_in_use = 0;
    pthread_rwlock_init(&company->list_rw_lock, NULL);

    for (i = 0; i < company->list_size; i++)
        company->list[i].actor = NULL;

exit:
    return company;
}

/*
 * Using ids, add the child to the parent. Append the child to the end of the
 * `linked-list' of children.
 * Expects the write lock to be acquired when calling this function.
 */
void
company_set_parents_child (Company *company, int parent_id, int child_id)
{
    Node *sibling;
    Node *parent = &company->list[parent_id];
    Node *child  = &company->list[child_id];
    int sibling_id = parent->first_child;

    /* if the parent has no children (first child not set) */
    if (sibling_id == -1) {
        parent->first_child = child_id;
        goto set_actor_child;
    }

    /* loop until we find the end of the children list */
    while (sibling_id >= 0) {
        sibling = &company->list[sibling_id];
        sibling_id = sibling->next_sibling;
    }

    sibling->next_sibling = child_id;

set_actor_child:
    child->parent = parent_id;
}

/*
 * Acquire the write lock on the Company list. Find a Node in the list which
 * isn't being used. The index of that node becomes the id of the Actor. Create
 * that Actor.
 *
 * If parent_id is greater than 0, add the created Actor as a child of the 
 * parent Actor whose id is parent_id.
 *
 * Return the id of the created Actor. Returns -1 if an error occurs.
 */
int
company_add_actor (Company *company, lua_State *L, int parent_id)
{
    Actor *actor;
    int i, id = -1;

    pthread_rwlock_wrlock(&company->list_rw_lock);
    if (company->nodes_in_use == company->list_size) {
        /* realloc list */
    }

    for (i = 0; i < company->list_size; i++)
        if (!company->list[i].actor)
            break;

    /* if we made it all the way without finding a free node somehow */
    if (i == company->list_size)
        goto release;

    actor = actor_create(L, i);
    if (!actor)
        goto release;

    company->nodes_in_use++;
    node_init(&company->list[i], actor);
    id = i;

    if (parent_id >= 0)
        company_set_parents_child(company, parent_id, id);

release:
    pthread_rwlock_unlock(&company->list_rw_lock);
    return id;
}

/*
 * Acquire the write lock on the Company list and cleanup any existing nodes
 * (and the Node's corresponding Actor). Then free memory for the Company list
 * and Company itself.
 */
void
company_close (Company *company)
{
    int i;

    pthread_rwlock_wrlock(&company->list_rw_lock);
    for (i = 0; i < company->list_size; i++) {
        if (company->list[i].actor) {
            int sibling_id = company->list[i].first_child;
            while (sibling_id >= 0) {
                printf("Actor %d had child %d\n", i, sibling_id);
                sibling_id = company->list[sibling_id].next_sibling;
            }
            printf("Actor %d was a child of %d\n", i, company->list[i].parent);

            node_cleanup(&company->list[i]);
        }
    }
    pthread_rwlock_unlock(&company->list_rw_lock);

    pthread_rwlock_destroy(&company->list_rw_lock);
    free(company->list);
    free(company);
    company = NULL;
}

/*
 * Verify given Actor is in Company and a valid pointer.  Increment the Actor's
 * reference count and return its id.  Returns -1 if Actor pointer given is
 * NULL or Actor doesn't belong to company.
 */
int
company_ref_actor (Company *company, Actor *actor)
{
    int id = -1;

    if (!actor)
        goto exit;

    pthread_rwlock_rdlock(&company->list_rw_lock);
    if (company->list[actor->id].actor == actor)
        id = actor->id;
    pthread_rwlock_unlock(&company->list_rw_lock);

exit:
    return id;
}

/*
 * Verify id is a valid id for company. Returns the Actor pointer that 
 * corresponds to the given id and decrements its reference count. Returns NULL
 * if the Actor doesn't exist for the given id or id isn't a valid index.
 */
Actor *
company_deref_actor (Company *company, int id)
{
    Actor *actor = NULL;

    if (id < 0)
        goto exit;

    pthread_rwlock_rdlock(&company->list_rw_lock);
    if (company->list[id].actor)
        actor = company->list[id].actor;
    pthread_rwlock_unlock(&company->list_rw_lock);

exit:
    return actor;
}
