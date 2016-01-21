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

#define NODE_FAMILY_MAX 4

enum NodeFamily { 
    NODE_PARENT, 
    NODE_NEXT_SIBLING, 
    NODE_PREV_SIBLING, 
    NODE_CHILD 
};

typedef struct Node {
    Actor *actor;
    int attached;
    int family[NODE_FAMILY_MAX];
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
static inline int
node_index_valid (Company *company, int id)
{
    int ret = 0;

    if (id < 0)
        goto exit;

    pthread_rwlock_rdlock(&company->rw_lock);
    ret = !(id >= company->list_size);
    pthread_rwlock_unlock(&company->rw_lock);

exit:
    return ret;
}

/*
 * Returns 0 and *does not* acquire the lock if id is not a valid index.
 * Acquires the write lock otherwise and returns 1.
 */
static inline int
node_write (Company *company, int id)
{
    if (!node_index_valid(company, id))
        return 0;
    pthread_rwlock_wrlock(&company->list[id].rw_lock);
    return 1;
}

/*
 * Returns 0 and *does not* acquire the lock if id is not a valid index.
 * Acquires the read lock otherwise and returns 1.
 */
static inline int
node_read (Company *company, int id)
{
    if (!node_index_valid(company, id))
        return 0;
    pthread_rwlock_rdlock(&company->list[id].rw_lock);
    return 1;
}

/*
 * *Does not* do anything if id isn't a valid index. Unlocks the read/write
 * lock otherwise.
 */
static inline void
node_unlock (Company *company, int id)
{
    if (!node_index_valid(company, id))
        return;
    pthread_rwlock_unlock(&company->list[id].rw_lock);
}

/*
 * This function verifies the family member reference is both a valid index and
 * a valid operating reference. A node can be operated when it is attached with
 * actor and detached with actor. If it is detached without an actor, it is
 * removed because it doesn't reference any operation.
 *
 * If the id given isn't valid, returns -1. If the member type isn't valid, 
 * returns -1. If the family member reference isn't valid as per above, returns
 * -1. Otherwise, this returns the id of the family member.
 */
int
node_family_member (Company *company, int id, enum NodeFamily member)
{
    int ret = -1;
    int f; /* family member id */

    if (!node_read(company, id))
        goto unlock;
            
    f = company->list[id].family[member];

    /* if the read is successful (*ptr is a valid index) */
    if (node_read(company, f)) {
        /* If the Node isn't attached and has no Actor, it isn't valid */
        if (!company->list[f].attached && company->list[f].actor == NULL) {
            /* immediately unlock calling node and acquire the write lock */
            node_unlock(company, id);
            node_write(company, id);
            company->list[id].family[member] = -1;
        } else {
            ret = f;
        }
        node_unlock(company, f);
    }

unlock:
    node_unlock(company, id);
    return ret;
}

/*
 * Setup a node (an element of the Company's list).
 */
void
node_init (Company *company, int id, Actor *actor)
{
    int i;

    if (!node_write(company, id))
        return;

    company->list[id].actor = actor;
    company->list[id].attached = 1;
    
    for (i = 0; i < NODE_FAMILY_MAX; i++) 
        company->list[id].family[i] = -1;

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
    int i;

    if (!node_write(company, id))
        return;

    actor_destroy(company->list[id].actor);

    company->list[id].actor = NULL;
    company->list[id].attached = 0;

    for (i = 0; i < NODE_FAMILY_MAX; i++) 
        company->list[id].family[i] = -1;

    node_unlock(company, id);
    pthread_rwlock_destroy(&company->list[id].rw_lock);
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
    pthread_rwlock_init(&company->rw_lock, NULL);

    for (i = 0; i < company->list_size; i++) {
        company->list[i].actor = NULL;
        company->list[i].attached = 0;
    }

exit:
    return company;
}

/*
 * Using ids, add the child to the parent. Append the child to the end of the
 * `linked-list' of children.
 */
void
company_set_parents_child (Company *company, int parent_id, int child_id)
{
    int next;
    int sibling = node_family_member(company, parent_id, NODE_CHILD);

    /* if the parent has no children (first child not set) */
    if (sibling == -1) {
        node_write(company, parent_id);
        company->list[parent_id].family[NODE_CHILD] = child_id;
        node_unlock(company, parent_id);
        goto set_node_child;
    }

    /* loop until we find the end of the children list */
    while (sibling >= 0) {
        next = node_family_member(company, sibling, NODE_NEXT_SIBLING);

        if (next == -1)
            break;

        sibling = next;
    }

    node_write(company, sibling);
    company->list[sibling].family[NODE_NEXT_SIBLING] = child_id;
    node_unlock(company, sibling);

set_node_child:
    node_write(company, child_id);
    company->list[child_id].family[NODE_PARENT] = parent_id;
    node_unlock(company, child_id);
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

    /* Acquire read lock on Company data and each node: finds the first free */
    pthread_rwlock_rdlock(&company->rw_lock);
    for (i = 0; i < company->list_size; i++) {
        if (node_read(company, i)) {
            if (!company->list[i].attached && company->list[i].actor == NULL)
                goto unlock_and_write;
            node_unlock(company, i);
        }
    }
unlock_and_write:
    /* Unlock the read locks on the Node we found and also the Company data */
    node_unlock(company, i);
    pthread_rwlock_unlock(&company->rw_lock);

    /* acquire the write lock which tries to do as little as possible */
    pthread_rwlock_wrlock(&company->rw_lock);
    company->nodes_in_use++;
    if (company->nodes_in_use >= company->list_size) {
        /* TODO: realloc list */
    }
    pthread_rwlock_unlock(&company->rw_lock);

    /* if the id isn't valid somehow, just error out */
    if (!node_write(company, i))
        goto release;

    actor = actor_create(L, i);
    if (!actor)
        goto release;

    /* finally, set the valid id we've found */
    id = i;
    node_init(company, id, actor);

    if (parent_id >= 0)
        company_set_parents_child(company, parent_id, id);

release:
    node_unlock(company, i); /* i can't be id because id isn't always set */
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
    int id;
    pthread_rwlock_wrlock(&company->rw_lock);
    for (id = 0; id < company->list_size; id++) {
        if (!node_write(company, id))
            continue;

        int sibling_id = company->list[id].family[NODE_CHILD];
        while (sibling_id >= 0) {
            printf("Actor %d had child %d\n", id, sibling_id);
            sibling_id = company->list[sibling_id].family[NODE_NEXT_SIBLING];
        }
        printf("Actor %d was a child of %d\n", id, 
                company->list[id].family[NODE_PARENT]);

        node_cleanup(company, id);
    }
    pthread_rwlock_unlock(&company->rw_lock);
    pthread_rwlock_destroy(&company->rw_lock);
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

    if (!node_read(company, actor->id))
        goto exit;

    if (company->list[actor->id].actor == actor)
        id = actor->id;

    node_unlock(company, actor->id);

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

    if (!node_read(company, id))
        goto exit;

    if (company->list[id].actor)
        actor = company->list[id].actor;

    node_unlock(company, id);

exit:
    return actor;
}
