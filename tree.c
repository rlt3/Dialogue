#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree.h"

#define NODE_FAMILY_MAX 4

enum ReturnType {
    ERROR = -3,
    NODE_ERROR = -2,
    NODE_INVALID = -1
};

typedef struct Node {
    void *data;
    int attached;
    int benched;
    int parent;
    int children[5];
    pthread_rwlock_t rw_lock;
    pthread_rwlock_t ref_lock;
    int ref_count;
} Node;

typedef struct Tree {
    pthread_rwlock_t rw_lock;
    int list_size;
    int list_max_size;
    int list_resize_factor;
    int root;
    data_cleanup_func_t cleanup_func;
    data_lookup_func_t lookup_func;
    data_set_id_func_t set_id_func;
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
tree_root ()
{
    int id = NODE_INVALID;
    if (tree_read() == 0) {
        id = global_tree->root;
        tree_unlock();
    }
    return id;
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
node_ref_count (int id)
{
    int count = -1;

    if (pthread_rwlock_rdlock(&global_tree->list[id].ref_lock) != 0)
        goto exit;

    count = global_tree->list[id].ref_count;
    pthread_rwlock_unlock(&global_tree->list[id].ref_lock);;

exit:
    return count;
}

int 
node_write (int id)
{
    if (!tree_index_is_valid(id))
        return 1;
    printf("\tWrite %d\n", id);
    return pthread_rwlock_wrlock(&global_tree->list[id].rw_lock);
}

int 
node_read (int id)
{
    if (!tree_index_is_valid(id))
        return 1;
    printf("Read %d\n", id);
    return pthread_rwlock_rdlock(&global_tree->list[id].rw_lock);
}

int 
node_unlock (int id)
{
    printf("Unlock %d\n", id);
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
 * Attach (and unbench) the node to the tree system and give it a reference to
 * hold.
 */
void
node_mark_attached_wr (int id, void *data)
{
    global_tree->list[id].attached = 1;
    global_tree->list[id].benched = 0;
    global_tree->list[id].data = data;
    global_tree->set_id_func(data, id);
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
 * Initialize the node at the id. Typically is only called when the tree is
 * initially created and everytime a realloc occurs producing unitialized
 * pointers.
 */
void
node_init_wr (int id)
{
    int i;
    node_mark_unused_wr(id);

    pthread_rwlock_init(&global_tree->list[id].rw_lock, NULL);
    pthread_rwlock_init(&global_tree->list[id].ref_lock, NULL);

    printf("%d: %p\n", id, &global_tree->list[id].rw_lock);

    global_tree->list[id].data = NULL;
    global_tree->list[id].ref_count = 0;
    global_tree->list[id].parent = NODE_INVALID;

    for (i = 0; i < 5; i++)
        global_tree->list[id].children[i] = NODE_INVALID;
}

/*
 * With the write lock on id:
 * Returns 0 if successful, 1 if parent_id is invalid.
 */
int
node_add_parent_wr (int id, int parent_id)
{
    return 0;
}

/*
 * Acquire the read lock first and make sure the node is marked for cleanup. If
 * the node is being used, has any references, exit with 1 for "unable to
 * cleanup".
 *
 * Otherwise acquire the write lock and cleanup the node according to the
 * cleanup_func which was given at tree_init and return 0 for "cleaned up".
 *
 * If the node has already been cleaned up exit with 0.
 */
int
node_cleanup (int id)
{
    int ret = 1;

    if (node_read(id) != 0)
        goto exit;

    if (node_is_used_rd(id))
        goto unlock;

    if (node_ref_count(id) > 0)
        goto unlock;

    if (global_tree->list[id].data == NULL)
        goto clean_exit;

    node_unlock(id);
    node_write(id);

    printf("Node %d\n", id);

    /* even tho we checked it above, we can't rely on it because we unlocked */
    if (global_tree->list[id].data) {
        global_tree->cleanup_func(global_tree->list[id].data);
        global_tree->list[id].data = NULL;
    }

clean_exit:
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
 * Have the tree take ownship of the pointer. The tree will cleanup that
 * pointer with the data_cleanup_func_t given in tree_init. 
 *
 * The tree attaches the pointer to a Node which is added as a child of
 * parent_id.  If parent_id <= NODE_INVALID then the Tree assumes that is
 * supposed to be the root Node and saves it (the root node has no parent).
 *
 * Returns the id of the Node inside the tree. 
 *
 * Returns NODE_INVALID if the tree was unable to allocate more memory for
 * Nodes to hold the reference.
 *
 * Returns NODE_ERROR if parent_id > -1 *and* the parent_id isn't in use (a
 * valid. 
 * 
 * Returns ERROR
 *      - if data is NULL
 *      - write-lock fails while setting the root node
 */
int
tree_add_reference (void *data, int parent_id)
{
    int max_id, id, set_root = 0, ret = ERROR;

    if (data == NULL)
        goto exit;

    if (parent_id <= NODE_INVALID)
        set_root = 1;

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
        ret = NODE_INVALID;
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

    /* 
     * If we are setting the root of the Node, we obviously can't be adding
     * it as a child of any particular node.
     */
    if (set_root) {
        if (tree_write() != 0) {
            ret = ERROR;
            goto unlock;
        }
        global_tree->root = id;
        tree_unlock();
    } else {
        if (node_add_parent_wr(id, parent_id) != 0) {
	    printf("%d couldn't be added to %d\n", id, parent_id);
    	    node_mark_unused_wr(id);
	    ret = ERROR;
	    goto unlock;
        }
    }

    ret = id;
unlock:
    node_unlock(id);
exit:
    return ret;
}

/*
 * Call the write capable function recursively over the tree given. Any node is
 * potentially a sub-tree. Calling this on the root of the entire tree will call
 * the function for each node of the tree.
 */
void
tree_write_map (int root, void (*write_capable_function) (int))
{
}

/*
 * Unlink the reference Node (by id) and all of its descendents in the tree.
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
    return 0;
}

/*
 * Initialze the tree.
 *
 * The tree's size will start as `length`. It will never exceed `max_length`.
 * It will resize itself by `scale_factor` (e.g. 2 doubles, 3 triples, etc).
 *
 * `cu` is the cleanup function called on all data given for reference. `lu` is
 * the lookup function on the data. 
 *
 * and the cleanup function for the
 * references (pointers it owns) it holds. 
 *
 * The `initial_length' also serves as
 * the base for resizing by a factor. So, if the length is 10, it is resized by
 * factors of 10.
 *
 * Returns 0 if no errors.
 */
int
tree_init (
        int length, 
        int max_length, 
        int scale_factor, 
        data_set_id_func_t set_id,
        data_cleanup_func_t cleanup,
        data_lookup_func_t lookup)
{
    int id, ret = 1;

    global_tree = malloc(sizeof(*global_tree));
    if (!global_tree)
        goto exit;

    global_tree->list = malloc(sizeof(Node) * length);
    if (!global_tree->list)
        goto exit;

    global_tree->cleanup_func = cleanup;
    global_tree->lookup_func = lookup;
    global_tree->set_id_func = set_id;
    global_tree->list_size = length;
    global_tree->list_max_size = max_length;
    global_tree->list_resize_factor = scale_factor;
    global_tree->root = NODE_INVALID;
    pthread_rwlock_init(&global_tree->rw_lock, NULL);

    for (id = 0; id < length; id++)
        node_init_wr(id);

    ret = 0;
exit:
    return ret;
}

/*
 * Mark all active Nodes as garbage and clean them up. Then free the memory for
 * the Tree and the list of Nodes.
 */
void
tree_cleanup ()
{
    int id, max_id = tree_list_size();

    /* The cleanup requires the Nodes to be garbage first */
    for (id = 0; id < max_id; id++) {
        node_write(id);
        node_mark_unused_wr(id);
        node_unlock(id);

        pthread_rwlock_wrlock(&global_tree->list[id].ref_lock);
        global_tree->list[id].ref_count = 0;
        pthread_rwlock_unlock(&global_tree->list[id].ref_lock);

        node_cleanup(id);
    }

    free(global_tree->list);
    free(global_tree);
}

/*
 * Get the pointer associated with the reference id. This function doesn't pass
 * ownership. 
 *
 * Returns NULL if the given id is bad either by having an invalid index or by
 * pointing to garbage data.
 *
 * Increments the ref_count for the Node at id. See node_cleanup.
 */
void *
tree_dereference (int id)
{
    void *ptr = NULL;

    if (node_read(id) != 0)
        goto exit;

    if (!node_is_used_rd(id))
        goto unlock;
    
    ptr = global_tree->list[id].data;

    pthread_rwlock_wrlock(&global_tree->list[id].ref_lock);
    global_tree->list[id].ref_count++;
    pthread_rwlock_unlock(&global_tree->list[id].ref_lock);

unlock:
    node_unlock(id);
exit:
    return ptr;
}

/*
 * Using the lookup_func, get the id for the Node from the data. Errors should
 * be handled through the lookup function.
 *
 * Decrements the ref_count for the Node at id. See node_cleanup.
 */
int
tree_reference (void *data)
{
    int id = global_tree->lookup_func(data);

    pthread_rwlock_wrlock(&global_tree->list[id].ref_lock);
    global_tree->list[id].ref_count--;
    pthread_rwlock_unlock(&global_tree->list[id].ref_lock);

    return id;
}

/*
 * Make, Lookup, Set Id, and Remove. Allocate some data and free it. Used to test we
 * don't have memory leaks.
 */

void* mk (int id) {
    return malloc(sizeof(int));
}

void set (void *data, int id) {
    *((int*)data) = id;
}

int lk (void *data) {
    return *((int*)data);
}

void rm (void *data) {
    free(data);
}

int 
main (int argc, char **argv)
{
    int status = 1;

    if (tree_init(10, 20, 2, set, rm, lk) != 0)
        goto exit;
    
    tree_add_reference(mk(0), NODE_INVALID);
    tree_add_reference(mk(1), 0);
    tree_add_reference(mk(2), 1);
    tree_add_reference(mk(3), 1);
    tree_add_reference(mk(4), 3);
    tree_add_reference(mk(5), 0);

    status = 0;
    tree_cleanup();
exit:
    return status;
}
