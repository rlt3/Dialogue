#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree.h"

#define NODE_FAMILY_MAX 4

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
    //printf("Write %d\n", id);
    return pthread_rwlock_wrlock(&global_tree->list[id].rw_lock);
}

int 
node_read (int id)
{
    if (!tree_index_is_valid(id))
        return 1;
    //printf("Read %d\n", id);
    return pthread_rwlock_rdlock(&global_tree->list[id].rw_lock);
}

int 
node_unlock (int id)
{
    //printf("Unlock %d\n", id);
    return pthread_rwlock_unlock(&global_tree->list[id].rw_lock);
}

/*
 * Acquire the write lock and cleanup the node according to the cleanup_func
 * which was given at tree_init. Also set the node's flags so it is marked as
 * unused.
 */
void
node_cleanup (int id)
{
    if (node_write(id) == 0) {
        global_tree->cleanup_func(global_tree->list[id].data);
        global_tree->list[id].data = NULL;
        global_tree->list[id].attached = 0;
        global_tree->list[id].benched = 0;
        node_unlock(id);
    }
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
 * With the write lock:
 * Initialize the node at the id. Typically is only called when the tree is initially 
 * created and everytime a realloc occurs producing unitialized pointers.
 */
void
node_init_wr (int id)
{
    global_tree->list[id].attached = 0;
    global_tree->list[id].benched = 0;
    pthread_rwlock_init(&global_tree->list[id].rw_lock, NULL);
}

/*
 * With the write lock:
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
 * Have the tree take ownship of the pointer. The tree will cleanup that pointer
 * with the data_cleanup_func_t given in tree_init.
 *
 * Returns the id of the reference inside the tree. Returns -1 if the tree was
 * unable to allocate more memory to hold the reference.
 */
int
tree_add_reference (void *data)
{
    int max_id, id;

find_unused_node:
    max_id = tree_list_size();

    /* Find the first unused node and then jump to get the write-lock on it */
    for (id = 0; id < max_id; id++) {
        if (node_read(id) == 0) {
            if (!node_is_used_rd(id))
                goto unlock_and_write;
            node_unlock(id);
        }
    }

    /* if we're here, no unused node was found */
    if (tree_resize() == 0) {
        goto find_unused_node;
    } else {
        id = -1;
        goto exit;
    }

unlock_and_write:
    node_unlock(id);
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
 * Returns 0 if successful.
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
     * Imagine three actors -- A, B, and C -- who are siblings of one another
     * with A being the first sibling (or first child of parent P) and we are
     * removing B.  If we don't lock A (the prev) while also locking B, we risk
     * a read on A's next, which is B (which should be invalid). We have to do
     * the same for C for going backwards (this is a doubly-linked list).
     */

    if (node_write(prev) == 0) {
        global_tree->list[prev].family[family] = next;
        node_unlock(prev);
    }

    if (node_write(next) == 0) {
        if (is_first_child)
            global_tree->list[next].family[NODE_PREV_SIBLING] = -1;
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
    int id;

    for (id = 0; id < global_tree->list_size; id++)
        node_cleanup(id);

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

    for (i = 0; i < 21; i++)
        printf("%d\n", tree_add_reference(NULL));

    status = 0;
    tree_cleanup();
exit:
    return status;
}
