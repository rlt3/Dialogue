#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree.h"

#define NODE_FAMILY_MAX 4

enum NodeType {
    NODE_ERROR = -2,
    NODE_INVALID = -1
};

enum NodeFamily { 
    NODE_PARENT, 
    NODE_NEXT_SIBLING, 
    NODE_PREV_SIBLING, 
    NODE_CHILD 
};

typedef struct Node {
    void *data;
    int attached;
    int benched;
    int family[NODE_FAMILY_MAX];
    pthread_rwlock_t rw_lock;
} Node;

typedef struct Tree {
    pthread_rwlock_t rw_lock;
    int list_size;
    int list_max_size;
    int list_resize_factor;
    data_cleanup_func_t cleanup_func;
    Node *list;
} Tree;

static Tree *global_tree = NULL;

int
tree_read ()
{
    return pthread_rwlock_rdlock(&global_tree->rw_lock);
}

int
tree_write ()
{
    return pthread_rwlock_wrlock(&global_tree->rw_lock);
}

int
tree_unlock ()
{
    return pthread_rwlock_unlock(&global_tree->rw_lock);
}

int
tree_list_size ()
{
    int ret = 0;
    if (tree_read() == 0) {
        ret = global_tree->list_size;
        tree_unlock();
    }
    return ret;
}

/*
 * Return 1 (true) or 0 (false) if the id is a valid index or not.
 *
 * Since we guarantee that the set of the valid indices always grows and never
 * shrinks, all we need to do is check that `id' is >= 0 and <= max-index and
 * the id will *always* be valid.
 */
int
tree_index_is_valid (int id)
{
    int ret = 0;

    if (id < 0)
        goto exit;

    if (tree_read() != 0)
        goto exit;

    ret = !(id > global_tree->list_size);
    tree_unlock();

exit:
    return ret;
}

int 
node_write (int id)
{
    if (!tree_index_is_valid(id))
        return 1;
    return pthread_rwlock_wrlock(&global_tree->list[id].rw_lock);
}

int 
node_read (int id)
{
    if (!tree_index_is_valid(id))
        return 1;
    return pthread_rwlock_rdlock(&global_tree->list[id].rw_lock);
}

int 
node_unlock (int id)
{
    return pthread_rwlock_unlock(&global_tree->list[id].rw_lock);
}

/*
 * With read or write lock:
 * Returns 1 (true) or 0 (false) if the node is being used or not.
 */
int
node_is_used_rd (int id)
{
    return (global_tree->list[id].attached || global_tree->list[id].benched);
}

/*
 * With a write lock:
 * Attach (and unbench) the node to the tree system and give it a reference to hold.
 */
void
node_mark_attached_wr (int id, void *data)
{
    global_tree->list[id].attached = 1;
    global_tree->list[id].benched = 0;
    global_tree->list[id].data = data;
}

/*
 * With a write lock:
 * Mark a node as benched. This keeps its reference id valid and its data from
 * being cleaned-up even if it removed from the tree.
 */
void
node_mark_benched_wr (int id)
{
    global_tree->list[id].attached = 0;
    global_tree->list[id].benched = 1;
}

/*
 * With a write lock:
 * Mark a node unusued, telling to system it is free to be cleaned-up.
 */
void
node_mark_unused_wr (int id)
{
    global_tree->list[id].attached = 0;
    global_tree->list[id].benched = 0;
}

/*
 * With a write lock:
 * Initialize the node at the id. Typically is only called when the tree is initially 
 * created and everytime a realloc occurs producing unitialized pointers.
 */
void
node_init_wr (int id)
{
    int i;
    node_mark_unused_wr(id);
    global_tree->list[id].data = NULL;
    for (i = 0; i < NODE_FAMILY_MAX; i++)
        global_tree->list[id].family[i] = NODE_INVALID;
    pthread_rwlock_init(&global_tree->list[id].rw_lock, NULL);
}

/*
 * Acquire the read lock first and make sure the node is marked for cleanup. If
 * the node is being used or has already been cleaned up, exit. Otherwise
 * acquire the write lock and cleanup the node according to the cleanup_func
 * which was given at tree_init.
 *
 * Returns 0 if the node is cleaned up and a free-to-use node, 1 otherwise.
 */
int
node_cleanup (int id)
{
    int ret = 1;

    if (node_read(id) != 0)
        goto exit;
    
    if (node_is_used_rd(id))
        goto unlock;

    /* if it isn't used and there's no data, it's good */
    if (!global_tree->list[id].data) {
        ret = 0;
        goto unlock;
    }

    node_unlock(id);
    node_write(id);
    global_tree->cleanup_func(global_tree->list[id].data);
    global_tree->list[id].data = NULL;
    ret = 0;
unlock:
    node_unlock(id);
exit:
    return ret;
}

/*
 * Acquires the write-lock for the tree. Resizes the list by the factor given
 * at tree_init-- 2 doubles its size, 3 triples, etc. Returns 0 if successful.
 */
int
tree_resize ()
{
    Node *memory = NULL;
    int id, size, ret = 1;

    tree_write();

    if (global_tree->list_size >= global_tree->list_max_size)
        goto unlock;

    id = global_tree->list_size;
    size = global_tree->list_size * global_tree->list_resize_factor;

    /* ceiling the size as the factor may cause it to overflow */
    if (size > global_tree->list_max_size)
        size = global_tree->list_max_size;

    memory = realloc(global_tree->list, size * sizeof(Node));

    if (memory != NULL) {
        ret = 0;
        global_tree->list = memory;
        global_tree->list_size = size;
        for (; id < global_tree->list_size; id++)
            node_init_wr(id);
    }

unlock:
    tree_unlock();
    return ret;
}

/*
 * Acquire the write lock on the parent. Add the child to that parent. If the
 * parent already has children, append the child to the end of the sibling
 * list.  Returns 0 if successful, 1 if either parent_id or child_id are
 * invalid.
 */
int
node_add_child (int parent_id, int child_id)
{
    int sibling, next = 0, ret = 1;

    /*
     * Lock parent for the duration of the function because we can't assume 
     * that the parent won't get disabled if we free its lock.
     */
    if (node_write(parent_id) != 0)
        goto exit;

    if (node_write(child_id) != 0)
        goto unlock;
    global_tree->list[child_id].family[NODE_PARENT] = parent_id;
    node_unlock(child_id);

    sibling = global_tree->list[parent_id].family[NODE_CHILD];

    /* if the parent has no children (first child not set) */
    if (sibling == NODE_INVALID) {
        global_tree->list[parent_id].family[NODE_CHILD] = child_id;
    } else {
    /* else there are children, so find the last sibling and append new child */
        while (sibling >= 0) {
            node_read(sibling);
            next = global_tree->list[sibling].family[NODE_NEXT_SIBLING];
            node_unlock(sibling);
            if (next == NODE_INVALID)
                break;
            sibling = next;
        }

        node_write(sibling);
        global_tree->list[sibling].family[NODE_NEXT_SIBLING] = child_id;
        node_unlock(sibling);
    }

    ret = 0;
unlock:
    node_unlock(parent_id);
exit:
    return ret;
}

/*
 * Have the tree take ownship of the pointer. The tree will cleanup that
 * pointer with the data_cleanup_func_t given in tree_init. 
 *
 * The tree attaches the pointer to a Node which is added as a child of
 * parent_id.  If parent_id <= NODE_INVALID then the Node won't have a parent.
 *
 * Returns the id of the Node inside the tree. 
 *
 * Returns NODE_INVALID if the tree was unable to allocate more memory for
 * Nodes to hold the reference.
 *
 * Returns NODE_ERROR if parent_id > -1 *and* the parent_id isn't in use (a
 * valid, non-garbage reference).
 */
int
tree_add_reference (void *data, int parent_id)
{
    int max_id, id = NODE_ERROR;

    if (parent_id > NODE_INVALID) {
        if (node_read(parent_id) != 0)
            goto exit;

        if (!node_is_used_rd(parent_id)) {
            node_unlock(parent_id);
            goto exit;
        }
        node_unlock(parent_id);
    }

find_unused_node:
    max_id = tree_list_size();

    /* find first unused node and clean it up if needed */
    for (id = 0; id < max_id; id++)
        if (node_cleanup(id) == 0)
            goto write;

    /* if we're here, no unused node was found */
    if (tree_resize() == 0) {
        goto find_unused_node;
    } else {
        id = NODE_INVALID;
        goto exit;
    }

write:
    node_write(id);

    /*
     * The id we found could theoretically be `found' by another thread, so
     * after acquiring the write-lock on it, we double-check it is unused or we
     * loop back to find another unused one.
     */
    if (node_is_used_rd(id)) {
        node_unlock(id);
        goto find_unused_node;
    }

    node_mark_attached_wr(id, data);
    node_unlock(id);

    if (parent_id > NODE_INVALID)
        node_add_child(parent_id, id);

exit:
    return id;
}

/*
 * Unlink the reference Node (by id) inside the tree and all its children.
 *
 * This function doesn't cleanup the reference data (given in 
 * tree_add_reference) for any of the nodes unlinked. 
 *
 * If `is_delete' is true, then the nodes will be marked for cleanup (which
 * happens in tree_add_reference) and the reference id will be invalid (should
 * be discarded).
 *
 * If `is_delete' is false, the nodes aren't marked for cleanup and the
 * reference id will still be valid meaning the node is unlinked from the tree
 * but still exists. This may be used for temporarily removing a reference from
 * the tree and then adding it back.
 *
 * Returns 0 if successful, 1 if the id is invalid.
 */
int
tree_unlink_reference (int id, int is_delete)
{
    int ret = 1;
    int prev, next, parent, child, family, is_first_child, next_sib, sibling;
    void (*unlink_func)(int);

    if (node_write(id) != 0)
        goto exit;

    if (is_delete)
        unlink_func = node_mark_unused_wr;
    else
        unlink_func = node_mark_benched_wr;

    parent = global_tree->list[id].family[NODE_PARENT];
    prev = global_tree->list[id].family[NODE_PREV_SIBLING];
    next = global_tree->list[id].family[NODE_NEXT_SIBLING];
    child = global_tree->list[id].family[NODE_CHILD];
    is_first_child = !(prev >= 0);

    unlink_func(id);

    if (child >= 0) {
        sibling = global_tree->list[id].family[NODE_CHILD];
        while (sibling >= 0) {
            if (node_write(sibling) == 0) {
                next_sib = global_tree->list[sibling].family[NODE_NEXT_SIBLING];
                unlink_func(sibling);
                node_unlock(sibling);
                sibling = next_sib;
            }
        }
    }

    /* 
     * So we can keep being DRY, The first child's prev `pointer' (which is -1)
     * ends up being the parent if you think of the parent as the head of a
     * doubly linked list. 
     */
    if (is_first_child) {
        prev = parent;
        family = NODE_CHILD;
    } else {
        family = NODE_NEXT_SIBLING;
    }

    /*
     *      P
     *      |
     *      V
     *   <- A <=> B <=> C ->
     *
     * Example: we need to remove B while some other part of the program is
     * reading down the list of P's children.
     *
     * If we don't lock A and B at the same time, we risk a read on A getting
     * B's id, then a read on B. Since we *must* have a write lock on B, that
     * read on B blocks and waits on what will become an invalid Node. When B 
     * is done blocking, it is invalid and it won't be able to point to C.  For
     * a period of time, all of P's children past B are unreachable.
     *
     * Locking both A and B at the same time prevents this from happening. Same
     * applies to C and B (for the reverse).
     */

    if (node_write(prev) == 0) {
        global_tree->list[prev].family[family] = next;
        node_unlock(prev);
    }

    if (node_write(next) == 0) {
        if (is_first_child)
            global_tree->list[next].family[NODE_PREV_SIBLING] = NODE_INVALID;
        else
            global_tree->list[next].family[NODE_PREV_SIBLING] = prev;
        node_unlock(prev);
    }

    ret = 0;
    node_unlock(id);
exit:
    return ret;
}

/*
 * Initialze the tree with the given length and the cleanup function for the
 * references (pointers it owns) it holds. The `initial_length' also serves as
 * the base for resizing by a factor. So, if the length is 10, it is resized by
 * factors of 10.
 *
 * Returns 0 if no errors.
 */
int
tree_init (int length, int max_length, int scale_factor, data_cleanup_func_t f)
{
    int id, ret = 1;

    global_tree = malloc(sizeof(*global_tree));
    if (!global_tree)
        goto exit;

    global_tree->list = malloc(sizeof(Node) * length);
    if (!global_tree->list)
        goto exit;

    global_tree->cleanup_func = f;
    global_tree->list_size = length;
    global_tree->list_max_size = max_length;
    global_tree->list_resize_factor = scale_factor;
    pthread_rwlock_init(&global_tree->rw_lock, NULL);

    for (id = 0; id < length; id++)
        node_init_wr(id);

    ret = 0;
exit:
    return ret;
}

/*
 * Acquire the write lock on the tree and then each of the nodes (one at a
 * time) and clean them up. Free the tree's memory.
 */
void
tree_cleanup ()
{
    int id, max_id;

    if (tree_write() != 0)
        goto free;
    /* 
     * get the last max range and all references (future and current) invalid
     * because no can fit in range -1 < id < -1 
     */
    max_id = global_tree->list_size;
    global_tree->list_size = NODE_INVALID;
    tree_unlock();

    for (id = 0; id < max_id; id++)
        node_cleanup(id);

free:
    free(global_tree->list);
    free(global_tree);
}

void
cu (void *data)
{
    return;
}

int 
main (int argc, char **argv)
{
    int i, status = 1;

    if (tree_init(10, 20, 2, cu) != 0)
        goto exit;

    //for (i = 0; i < 21; i++)
    //    printf("%d\n", tree_add_reference(NULL, NODE_INVALID));
    
    printf("%d\n", tree_add_reference(NULL, NODE_INVALID));
    printf("%d\n", tree_add_reference(NULL, 0));
    printf("%d\n", tree_add_reference(NULL, 0));
    printf("%d\n", tree_add_reference(NULL, 1));

    status = 0;
    tree_cleanup();
exit:
    return status;
}
