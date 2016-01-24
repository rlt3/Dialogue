#include <stdlib.h>
#include <pthread.h>
#include "company.h"

#define NODE_FAMILY_MAX 4

enum NodeFamily { 
    NODE_PARENT, 
    NODE_NEXT_SIBLING, 
    NODE_PREV_SIBLING, 
    NODE_CHILD 
};

enum RetVals {
    BAD_OPT = -3,
    BAD_NODE = -2
};

typedef struct Node {
    Actor *actor;  /* if not attached, should be benched */
    int attached; /* if attached, should be not benched */
    int benched;  /* if not attached & not benched, it is a unused Node */
    int family[NODE_FAMILY_MAX];
    pthread_rwlock_t rw_lock;
} Node;

struct Company {
    Node *list;
    int list_size;
    int list_inc;
    int nodes_in_use;
    pthread_rwlock_t rw_lock;
};

/*
 * Make sure the id is a valid index so we aren't accessing memory out of 
 * bounds. If id is less than 0, we can assume an invalid index. Otherwise, we
 * have to do a read lock on the list's size.
 *
 * If we stay in the bounds of list_size and 0, all ids are guaranteed to
 * be valid (wether the Node is valid or not is another matter). This means
 * we don't have to acquire the read lock to read the list of Nodes, but
 * need to acquire the read lock of any Node.
 */
static inline int
node_index_valid (Company *company, int id)
{
    int ret = 0;

    if (id < 0)
        goto exit;

    pthread_rwlock_rdlock(&company->rw_lock);
    ret = !(id > company->list_size);
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
    if (node_index_valid(company, id))
        pthread_rwlock_unlock(&company->list[id].rw_lock);
}

/*
 * This is effectively the garbage collection.
 */
void
node_unlink (Company *company, int id, int unlink_sibling)
{
    int prev, next, parent, child, fam, is_first_child, has_children, has_next;

    if (!node_read(company, id))
        return;

    prev = company->list[id].family[NODE_PREV_SIBLING];
    next = company->list[id].family[NODE_NEXT_SIBLING];
    parent = company->list[id].family[NODE_PARENT];
    child = company->list[id].family[NODE_CHILD];

    is_first_child = !(prev >= 0);
    has_children = (child >= 0);
    has_next = (next >= 0);

    /* 
     * The first child's prev `pointer' ends up being the parent if you think
     * of the parent as the head of a doubly linked list.
     */
    if (is_first_child) {
        prev = parent;
        fam = NODE_CHILD;
    } else {
        fam = NODE_NEXT_SIBLING;
    }

    node_unlock(company, id);

    if ((is_first_child || has_next) && node_write(company, prev)) {
        company->list[prev].family[fam] = next;
        node_unlock(company, prev);
    }

    if (has_next && node_write(company, next)) {
        company->list[next].family[NODE_PREV_SIBLING] = (is_first_child ? -1 : prev);
        node_unlock(company, prev);
        if (unlink_sibling)
            node_unlink(company, next, unlink_sibling);
    }

    if (has_children && node_write(company, id)) {
        /* I think simply setting the first_child to -1 right now will keep 
         * a lot of work from us. Doing that will let Nodes fall to the wayside,
         * the Actor address (id) still valid. They aren't active, thus will
         * not receive messages, and will be garbage collected when an needs to
         * be created.
         */
        node_unlock(company, id);
    }

}

/*
 * This function verifies the family member reference is both a valid index and
 * a valid operating reference. A node can be operated when it is attached with
 * actor and detached with actor. If it is detached without an actor, it is
 * removed because it doesn't reference any operation.
 *
 * If the id given isn't valid, returns BAD_NODE.
 *
 * If the family member reference (not its type) isn't valid as per above,
 * returns -1.  Otherwise, this returns the id of the family member.
 */
int
node_family_member (Company *company, int id, enum NodeFamily member)
{
    int ret = -1;
    int f; /* family member id */

    if (!node_read(company, id)) {
        ret = BAD_NODE;
        goto unlock;
    }
            
    f = company->list[id].family[member];

    /*
     * I don't think this next portion of code is valid anymore. Actors that
     * are being deleted (because an Action for it to be deleted was sent) are
     * unlinked then. When another actor is created, it sees the unlinked 
     * actor, frees its memory, and replaces it, attached and unbenched this
     * time too.
     *
     * An actor is unbenched (if it was) and detached (if it was) which then
     * removes itself from any further messages. This seems the most efficient
     * way of dealing with creation and deletion -- at the same time.
     */

    /* if the read is successful (*ptr is a valid index) */
    if (node_read(company, f)) {
        /* If the Node isn't attached and has no Actor, it isn't valid */
        if (!company->list[f].attached && !company->list[f].benched) {
            /* immediately unlock read lock and acquire the write lock */
            node_unlock(company, id);
            node_write(company, id);

            company->list[id].family[member] = -1;

            /*
             * TODO:
             *  make sure next and previous are linked. Probably using doubly
             *  linked list here.
             *
             * TODO:
             *  if we add a is_working bool, we can sweep references which are
             *  pointing to to-be deleted actors. we can do garbage collection
             *  in this way.
             */
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
    Node *array;
    int i;

    if (!node_write(company, id))
        return;

    /* acquire the write lock and try to get out as fast as possible. */
    pthread_rwlock_wrlock(&company->rw_lock);

    if (company->nodes_in_use == company->list_size) {
        array = realloc(company->list, 
                (company->list_size + company->list_inc) * sizeof(Node));

        /* If realloc fails, our current memory layout is still valid */
        if (array) {
            company->list_size += company->list_inc;
            company->nodes_in_use = company->list_size;
        }
    } else {
        company->nodes_in_use++;
    }

    pthread_rwlock_unlock(&company->rw_lock);

    company->list[id].actor = actor;
    company->list[id].attached = 1;
    
    for (i = 0; i < NODE_FAMILY_MAX; i++) 
        company->list[id].family[i] = -1;

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

    if (company->list[id].actor)
        actor_destroy(company->list[id].actor);

    /* acquire the write lock which tries to do as little as possible */
    pthread_rwlock_wrlock(&company->rw_lock);
    company->nodes_in_use--;
    pthread_rwlock_unlock(&company->rw_lock);

    company->list[id].actor = NULL;
    company->list[id].attached = 0;

    for (i = 0; i < NODE_FAMILY_MAX; i++) 
        company->list[id].family[i] = -1;

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
    company->list_inc = buffer_length;
    company->nodes_in_use = 0;
    pthread_rwlock_init(&company->rw_lock, NULL);

    for (i = 0; i < company->list_size; i++) {
        company->list[i].actor = NULL;
        company->list[i].attached = 0;
        pthread_rwlock_init(&company->list[i].rw_lock, NULL);
    }

exit:
    return company;
}

/*
 * Using IDs, add the child to the parent. If either of the child or parent
 * ids are invalid, it returns early with BAD_NODE. Otherwise it returns 0. 
 *
 * Adds child to parent's first_child slot if it's its first child.  Append the
 * child to the end of the `linked-list' of siblings and set the child's parent
 * to the given parent.
 */
int
company_parent_add_child (Company *company, int parent_id, int child_id)
{
    int next;
    int sibling = node_family_member(company, parent_id, NODE_CHILD);

    /* if the parent was bad */
    if (sibling == BAD_NODE)
        goto error;

    /* if the child is bad */
    if (!node_write(company, child_id))
        goto error;
    company->list[child_id].family[NODE_PARENT] = parent_id;
    node_unlock(company, child_id);

    /* if the parent has no children (first child not set) */
    if (sibling == -1) {
        node_write(company, parent_id);
        company->list[parent_id].family[NODE_CHILD] = child_id;
        node_unlock(company, parent_id);
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

    return 0;
error:
    return BAD_NODE;
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
    int i, list_size, id = -1;

    /* 
     * Even if parent isn't valid, we can still create the Actor. So we set
     * this to -1 and just check to make sure it is 0 or greater later.
     */ 
    if (!node_index_valid(company, parent_id))
        parent_id = -1;

find_unused_node:
    pthread_rwlock_rdlock(&company->rw_lock);
    list_size = company->list_size;
    pthread_rwlock_unlock(&company->rw_lock);

    /* Acquire read lock on Company data and each node: finds the first free */
    for (i = 0; i < list_size; i++) {
        if (node_read(company, i)) {
            if (!company->list[i].attached && company->list[i].actor == NULL)
                goto unlock_and_write;
            node_unlock(company, i);
        }
    }

unlock_and_write:
    node_unlock(company, i);

    /* it's a serious error if somehow i isn't a valid index now */
    if (!node_write(company, i))
        goto release;

    /*
     *  Theoretically, the id we found `i', could be locked and 'messed with'
     *  before we acquire the write thread. So, we unlock the write lock, then
     *  go and find a new one.
     */
    if (company->list[i].attached || company->list[i].actor) {
        node_unlock(company, i);
        goto find_unused_node;
    }

    actor = actor_create(L, i);
    if (!actor)
        goto release;

    /* finally, set the valid id we've found */
    id = i;
    node_init(company, id, actor);

    if (parent_id >= 0)
        company_parent_add_child(company, parent_id, id);

release:
    node_unlock(company, i); /* i can't be id because id isn't always set */
    return id;
}

void
print_children (Company *company, int id)
{
    int prev_sibling;
    int sibling = company->list[id].family[NODE_CHILD];
    while (sibling >= 0) {
        printf(" %d ", sibling);
        if (node_read(company, sibling)) {
            prev_sibling = sibling;
            sibling = company->list[sibling].family[NODE_NEXT_SIBLING];
            node_unlock(company, prev_sibling);
        }
    }
    printf("\n");
}

/*
 * Acquire the write lock on the Company list and cleanup any existing nodes
 * (and the Node's corresponding Actor). Then free memory for the Company list
 * and Company itself.
 */
void
company_close (Company *company)
{
    int list_size, is_working, id;

    pthread_rwlock_rdlock(&company->rw_lock);
    list_size = company->list_size;
    pthread_rwlock_unlock(&company->rw_lock);

    for (id = 0; id < list_size; id++) {
        node_read(company, id);
        is_working = !(!company->list[id].attached && 
                       company->list[id].actor == NULL);

        if (!is_working)
            goto cleanup;

        printf("Actor %d\n", id);
        print_children(company, id);
        printf("  [fc] -> %d\n", company->list[id].family[NODE_CHILD]);
        printf("  [nc] -> %d\n", company->list[id].family[NODE_NEXT_SIBLING]);
        printf("  [pa] -> %d\n", company->list[id].family[NODE_PARENT]);

cleanup:
        node_unlock(company, id);
        node_cleanup(company, id);
        pthread_rwlock_destroy(&company->list[id].rw_lock);
    }

    pthread_rwlock_destroy(&company->rw_lock);
    free(company->list);
    free(company);
    company = NULL;
}

/*
 * Verify given Actor is in Company and a valid pointer.  Returns -1 if Actor
 * pointer given is NULL or Actor doesn't belong to company.
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
 * corresponds to the given id.  Returns NULL if the Actor doesn't exist for
 * the given id or id isn't a valid index.
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
