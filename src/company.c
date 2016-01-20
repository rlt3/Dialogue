#include <stdlib.h>
#include <pthread.h>
#include "company.h"

/*
 * We have two main read/write locks. One for each Node and another for the
 * list of the Nodes themselves. 
 *
 * Since each Node only explicitly knows one Node in each direction (up
 * [parent], right [next sibling], down [first child]), when removing a Node
 * (R) from the tree, we can use a write lock only on the Nodes which reference
 * it rather than the entire tree itself. So, all Nodes which have R as a
 * parent, the previous sibling node to a Node, or parent which has it as a
 * first child.
 *
 * Once the Node has been removed from the tree (making it dead to all 
 * operations of that tree), it can be destroyed. This is where the write lock
 * of the list comes into play: for destroying the removed actor.
 */

typedef struct Node {
    Actor *actor;

    int ref_count;
    pthread_mutex_t ref_mutex;

    /* these are indices to other Nodes */
    int parent;
    int next_sibling;
    int first_child;
    pthread_rwlock_t family_rw_lock;
} Node;

struct Company {
    Node *list;
    int list_size;
    int nodes_in_use;
    pthread_rwlock_t list_rw_lock;
};

/*
 * Setup a node (an element of the Company's list).
 */
void
node_init (Node *node, Actor *actor)
{
    node->actor = actor;
    node->parent = -1;
    node->next_sibling = -1;
    node->first_child = -1;
    pthread_mutex_init(&node->ref_mutex, NULL);
    pthread_rwlock_init(&node->family_rw_lock, NULL);
}

/*
 * Cleanup a node, destroying the Actor and setting its pointer to NULL,
 * set references to zero, and destroying the ref mutex and family rwlock.
 */
void
node_cleanup (Node *node)
{
    actor_destroy(node->actor);
    node->actor = NULL;
    node->parent = -1;
    node->next_sibling = -1;
    node->first_child = -1;
    pthread_mutex_destroy(&node->ref_mutex);
    pthread_rwlock_destroy(&node->family_rw_lock);
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
    if (company->list[actor->id].actor == actor) {
        id = actor->id;

        pthread_mutex_lock(&company->list[id].ref_mutex);
        company->list[id].ref_count++;
        pthread_mutex_unlock(&company->list[id].ref_mutex);
    }
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
    if (company->list[id].actor) {
        actor = company->list[id].actor;

        pthread_mutex_lock(&company->list[id].ref_mutex);
        company->list[id].ref_count--;
        pthread_mutex_unlock(&company->list[id].ref_mutex);
    }
    pthread_rwlock_unlock(&company->list_rw_lock);

exit:
    return actor;
}
