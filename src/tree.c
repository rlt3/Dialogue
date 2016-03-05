#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree.h"

#define NODE_FAMILY_MAX 4

typedef struct Node {
    void *data;

    /* TODO: represent the three states in a single member */
    int attached;
    int benched;

    /*
     * TODO: can have the children array below be a generic list of ids where
     * the 0th element is always the parent and >0 are the children.
     */
    int parent;

    /* TODO: an array with the next index (`last_child` + 1) and the max
     * children */
    int *children;
    int last_child;
    int max_children;

    int thread_id;

    /* rwlock everything that isn't the data */
    pthread_rwlock_t rw_lock;

    /*
     * mutex for the data. garbage collection requires both rwlock & mutex to
     * be acquired 
     */
    pthread_mutex_t data_lock;
} Node;

typedef struct Tree {
    Node *list;

    /* the size of the tree's list right now */
    int list_size;

    /* the ceiling of the tree's list after resizing */
    int list_max_size;

    /* resize by 2 times, 3 times, n times ... */
    int list_resize_factor;

    /* id to the root of the tree (not always 0) */
    int root;

    /* the function responsible for garbage collecting Node data */
    data_cleanup_func_t cleanup_func;

    /* the function for setting the id in the given data */
    data_set_id_func_t set_id_func;

    /* everything is protected by the rwlock except the list */
    pthread_rwlock_t rw_lock;
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

int
node_data_lock (int id)
{
    return pthread_mutex_lock(&global_tree->list[id].data_lock);
}

int
node_data_trylock (int id)
{
    return pthread_mutex_trylock(&global_tree->list[id].data_lock);
}

int
node_data_unlock (int id)
{
    return pthread_mutex_unlock(&global_tree->list[id].data_lock);
}

/*
 * Must be called with the write lock (structure) *and* the mutex lock (data)
 * acquired for the node.  Attach (and unbench) the node to the tree system and
 * give it a reference to hold.
 */
void
node_mark_attached_fullwr (int id, void *data, int thread_id)
{
    global_tree->list[id].attached = 1;
    global_tree->list[id].benched = 0;
    global_tree->list[id].data = data;
    global_tree->list[id].thread_id = thread_id;
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
    global_tree->list[id].parent = NODE_INVALID;
}

/*
 * With a write lock:
 * Initialize the node at the id. Typically is only called when the tree is
 * initially created and everytime a realloc occurs producing unitialized
 * pointers.
 */
int
node_init_wr (int id)
{
    int i, length = 5, ret = 1;

    node_mark_unused_wr(id);
    pthread_rwlock_init(&global_tree->list[id].rw_lock, NULL);
    pthread_mutex_init(&global_tree->list[id].data_lock, NULL);

    global_tree->list[id].children = malloc(length * sizeof(int));
    if (!global_tree->list[id].children)
        goto exit;

    global_tree->list[id].data = NULL;
    global_tree->list[id].parent = NODE_INVALID;
    global_tree->list[id].max_children = length;
    global_tree->list[id].last_child = 0;

    for (i = 0; i < length; i++)
        global_tree->list[id].children[i] = NODE_INVALID;

    ret = 0;
exit:
    return ret;
}

/*
 * Must be called with the write lock (structure) *and* the mutex lock (data)
 * acquire for the node.
 * Cleanup the node's data with the cleanup_func given at tree_init.
 */
void
node_cleanup_fullwr (int id)
{
    int i;

    if (global_tree->list[id].data) {
        global_tree->cleanup_func(global_tree->list[id].data);
        global_tree->list[id].data = NULL;
    }

    for (i = 0; i < global_tree->list[id].max_children; i++)
        global_tree->list[id].children[i] = NODE_INVALID;

    global_tree->list[id].thread_id = NODE_INVALID;
}

/*
 * Must be called with the write lock (structure) *and* the mutex lock (data)
 * acquire for the node.
 * Destroy the memory for the node's data & children.
 */
void
node_destroy_fullwr (int id)
{
    node_cleanup_fullwr(id);

    if (global_tree->list[id].children) {
        free(global_tree->list[id].children);
        global_tree->list[id].children = NULL;
    }
}

/*
 * Acquire the write lock on the given Node to add the given child. Doesn't 
 * check the child id. If it successfully adds the child, returns 0. Returns 1 
 * otherwise.
 */
int
node_add_child (int id, int child)
{
    int cid, max_id, ret = 1;

    if (node_write(id) != 0)
        goto exit;

    if (!node_is_used_rd(id))
        goto unlock;

    max_id = global_tree->list[id].max_children;

    for (cid = 0; cid < max_id; cid++) {
        if (global_tree->list[id].children[cid] == NODE_INVALID) {
            global_tree->list[id].children[cid] = child;

            if (cid == global_tree->list[id].last_child)
                global_tree->list[id].last_child++;

            ret = 0;
            break;
        }
    }

    if (cid == max_id && ret == 1) {
        /* TODO: Reallocate node memory */
    }

unlock:
    node_unlock(id);
exit:
    return ret;
}

/*
 * Acquire the write lock on the given Node to remove the given child. If the
 * child is removed, returns 0. Otherwise (not found, incorrect id, etc) 
 * returns 1.
 */
int
node_remove_child (int id, int child)
{
    int cid, max_id, ret = 1;

    if (node_write(id) != 0)
        goto exit;
    
    max_id = global_tree->list[id].max_children;

    for (cid = 0; cid < max_id; cid++) {
        if (global_tree->list[id].children[cid] == child) {
            global_tree->list[id].children[cid] = NODE_INVALID;

            if (global_tree->list[id].last_child == cid)
                global_tree->list[id].last_child--;

            ret = 0;
            break;
        }
    }

    node_unlock(id);
exit:
    return ret;
}

/*
 * Cleanup the Node from the given id.
 * Returns 1 if it wasn't able to cleanup the Node.
 * Returns 0 if successful.
 */
int
node_cleanup (int id)
{
    int is_used, ret = 1;

    if (node_read(id) != 0)
        goto exit;
    is_used = node_is_used_rd(id);
    node_unlock(id);

    if (is_used)
        goto exit;

    if (node_data_trylock(id) != 0)
        goto exit;

    if (node_write(id) != 0)
        goto unlock_data;

    /* node can get changed after unlocking before, verify it's used */
    if (node_is_used_rd(id))
        goto unlock_node;

    node_cleanup_fullwr(id);
    ret = 0;
unlock_node:
    node_unlock(id);
unlock_data:
    node_data_unlock(id);
exit:
    return ret;
}

/*
 * Acquires the write-lock for the tree. Resizes the list by the factor given
 * at tree_init -- 2 doubles its size, 3 triples, etc. Returns 0 if successful.
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
        
        /* TODO: what if node_init fails here */
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
 * parent_id.  
 *
 * If parent_id <= NODE_INVALID then the Tree -- the firs time -- assumes that
 * is supposed to be the root Node and saves it (the root node has no parent).
 * Every time after that if the parent_id <= NODE_INVALID then the Node created
 * as a child of the root node.
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
tree_add_reference (void *data, int parent_id, int thread_id)
{
    int max_id, id, invalid_parent = 0, ret = ERROR;

    if (data == NULL)
        goto exit;

    if (parent_id <= NODE_INVALID)
        invalid_parent = 1;

find_unused_node:
    max_id = tree_list_size();

    /* find first unused node and clean it up if needed */
    for (id = 0; id < max_id; id++)
        if (node_cleanup(id) == 0)
            goto data_lock_and_write;

    /* if we're here, no unused node was found */
    if (tree_resize() == 0) {
        goto find_unused_node;
    } else {
        ret = NODE_INVALID;
        goto exit;
    }

data_lock_and_write:
    /* 
     * We set the lock-order of the Node's data lock before the Node's
     * structure write lock. This is so later (in node_cleanup) we can safely
     * do a trylock on the data without wasting time acquiring the write lock 
     * on the Node to then do a trylock.
     */
    node_data_lock(id);
    node_write(id);

    /*
     * The id we found could theoretically be `found' by another thread, so
     * after acquiring the write-lock on it, we double-check it is unused or we
     * loop back to find another unused one.
     */
    if (node_is_used_rd(id)) {
        node_unlock(id);
        node_data_unlock(id);
        goto find_unused_node;
    }

    node_mark_attached_fullwr(id, data, thread_id);

    /* 
     * The first time invalid_parent is true, the id found becomes the root
     * node for the tree. Everytime after that, the node which has an invalid
     * parent becomes the child of the root node. Only the root node has a
     * parent of NODE_INVALID. All other parents will be valid indices.
     */
    if (invalid_parent) {
        if (tree_write() != 0) {
            ret = ERROR;
            goto unlock;
        }
        
        /*
         * TODO:
         * A similar bug which caused the branch below arises when you try to
         * delete the root node. You can't delete the root right now.
         */

        if (global_tree->root > NODE_INVALID) {
            parent_id = global_tree->root;
            tree_unlock();
        } else {
            global_tree->root = id;
            tree_unlock();
            goto success;
        }
    }

    if (node_add_child(parent_id, id) != 0) {
        /* 
         * keep the allocated data but still return with an error. it will be
         * cleaned up automatically if we simply mark it as garbage.
         */
        node_mark_unused_wr(id);
        ret = NODE_ERROR;
        goto unlock;
    }

success:
    global_tree->list[id].parent = parent_id;
    ret = id;

unlock:
    node_unlock(id);
    node_data_unlock(id);

exit:
    return ret;
}

/*
 * Map the function to the subtree starting at the given root node. If is_read
 * is true, then read locks will be used. Otherwise write locks will be used.
 *
 * If is_recurse is tree, will go down the list of children calling the 
 * for all descendents of the initial root function. If is_recurse is false 
 * then the function will only do the given root and its direct children.

 * Keeps a stack-allocated list of 10 potential children. It acquires the write
 * lock on the root, calls the function, and gets the children. This is so we
 * can keep the locking policy of: children are locked before their parents are
 * locked. This policy is implicity defined in tree_add_reference.
 */
void
tree_map_subtree (int root, void (*function) (int), int is_read, int is_recurse)
{
    int (*lock_func)(int);
    /* 
     * TODO: create map_max_stack as a global var protected by an rwlock and the
     * largest children length for any given node. avoiding having to malloc
     * here is a c99 feature, e.g. int children[map_max_stack]; or we have set
     * a hard limit on the number of scripts.
     */
    int children[10];
    int max_id, id, cid = 0;

    if (is_read)
        lock_func = node_read;
    else
        lock_func = node_write;

    if (lock_func(root) != 0)
        return;

    if (!node_is_used_rd(root)) {
    	node_unlock(root);
        return;
    }

    memset(&children, -1, sizeof(int) * 10);
    max_id = global_tree->list[root].last_child;
    function(root);

    for (id = 0; id < max_id; id++) {
        if (global_tree->list[root].children[id] > NODE_INVALID) {
            children[cid] = global_tree->list[root].children[id];
            cid++;
        }
    }

    node_unlock(root);

    for (id = 0; id < cid; id++) {
        if (is_recurse) {
            tree_map_subtree(children[id], function, is_read, is_recurse);
        } else {
            if (lock_func(children[id]) == 0) {
                if (node_is_used_rd(root))
                    function(children[id]);
                node_unlock(children[id]);
            }
        }
    }
}

/*
 * Unlink a Node and all of its descendents from the tree. If is_delete is 1,
 * this will mark all the nodes unlinked as garbage so they can be cleaned-up 
 * and used by the system.
 *
 * If is_delete is 0 then the nodes are benched and aren't attached to the tree
 * but are otherwise still around and are not marked as garbage.
 */
int
tree_unlink_reference (int id, int is_delete)
{
    const int write = 0;
    const int recurse = 1;
    int parent, ret = 1;
    void (*unlink_func)(int);

    if (node_write(id) != 0)
        goto exit;

    parent = global_tree->list[id].parent;

    if (is_delete)
        unlink_func = node_mark_unused_wr;
    else
        unlink_func = node_mark_benched_wr;

    if (node_remove_child(parent, id) != 0)
        goto unlock;

    ret = 0;
unlock:
    node_unlock(id);

    if (ret == 0)
        tree_map_subtree(id, unlink_func, write, recurse);
exit:
    return ret;
}

/*
 * Re-link a (benched) reference back into the tree. 
 * Returns 0 if successful.
 * Returns 1 if the there was an error re-linking back to its parent.
 * Returns 2 if the node wasn't benched.
 * Returns 3 if the node was invalid.
 */
int
tree_link_reference (int id)
{
    int ret = 3;

    if (node_write(id) != 0)
        goto exit;

    if (global_tree->list[id].attached) {
        ret = 2;
        goto unlock;
    }

    if (node_add_child(global_tree->list[id].parent, id) != 0) {
        ret = 1;
        goto unlock;
    }

    global_tree->list[id].attached = 1;
    global_tree->list[id].benched = 0;

    ret = 0;
unlock:
    node_unlock(id);
exit:
    return ret;
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
        data_cleanup_func_t cleanup)
{
    int id, ret = 1;

    global_tree = malloc(sizeof(*global_tree));
    if (!global_tree)
        goto exit;

    global_tree->list = malloc(sizeof(Node) * length);
    if (!global_tree->list) {
        free(global_tree);
        goto exit;
    }

    global_tree->cleanup_func = cleanup;
    global_tree->set_id_func = set_id;
    global_tree->list_size = length;
    global_tree->list_max_size = max_length;
    global_tree->list_resize_factor = scale_factor;
    global_tree->root = NODE_INVALID;
    pthread_rwlock_init(&global_tree->rw_lock, NULL);

    for (id = 0; id < length; id++) {
        if (node_init_wr(id) != 0) {
            free(global_tree->list);
            free(global_tree);
            goto exit;
        }
    }

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

    for (id = 0; id < max_id; id++) {
        node_data_lock(id);
        node_write(id);
        node_destroy_fullwr(id);
        node_unlock(id);
        node_data_unlock(id);
    }

    free(global_tree->list);
    free(global_tree);
}

/*
 * Returns the thread id for the node of the given id.
 * Returns NODE_ERROR if an error occurs (bad node, etc)
 */
int
tree_reference_thread (int id)
{
    int thread = NODE_ERROR;
    if (node_read(id) != 0)
        goto exit;
    thread = global_tree->list[id].thread_id;
    node_unlock(id);
exit:
    return thread;
}

/*
 * Get the data (pointer) referenced by the id. 
 *
 * Returns NULL if the id is invalid, the node of the id is garbage, or the
 * data itself is NULL. Calling `tree_deref` isn't necessary (and is undefined)
 * if this function returns NULL.
 *
 * Returns the pointer to the data if the node is valid and has data. Calling
 * `tree_deref` is required if this function returns non-NULL data.
 */
void *
tree_ref (int id)
{
    int used;
    void *data = NULL;

    if (node_read(id) != 0)
        goto exit;

    used = node_is_used_rd(id);
    node_unlock(id);

    if (!used)
        goto exit;

    if (node_data_lock(id) != 0)
        goto exit;

    if (!global_tree->list[id].data) {
        node_data_unlock(id);
        goto exit;
    }

    data = global_tree->list[id].data;
exit:
    return data;
}

/*
 * Free up the data for some other process. If `tree_ref' returned NULL,
 * calling this function produces undefined behavior.
 */
int
tree_deref (int id)
{
    return node_data_unlock(id);
}
