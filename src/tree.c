#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree.h"

#define NODE_FAMILY_MAX 4
#define MAP_MAX_STACK 10

typedef struct Node Node;
typedef struct Tree Tree;
static Tree *global_tree = NULL;

struct Node {
    void *data;

    /* TODO: represent benched, attached, garbage in single member */
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
};

struct Tree {
    Node *list;

    /* the size of the tree's list right now */
    int list_size;

    /* id to the root of the tree (not always 0) */
    int root;

    /* the function responsible for garbage collecting Node data */
    data_cleanup_func_t cleanup_func;

    /* the function for setting the id in the given data */
    data_set_id_func_t set_id_func;

    /* everything is protected by the rwlock except the list */
    pthread_rwlock_t rw_lock;
};

static inline int
tree_read ()
{
    return pthread_rwlock_rdlock(&global_tree->rw_lock);
}

static inline int
tree_write ()
{
    return pthread_rwlock_wrlock(&global_tree->rw_lock);
}

static inline int
tree_unlock ()
{
    return pthread_rwlock_unlock(&global_tree->rw_lock);
}

/*
 * Return 1 (true) or 0 (false) if the id is a valid index or not.
 *
 * Since we guarantee that the set of the valid indices always grows and never
 * shrinks, all we need to do is check that `id' is >= 0 and <= max-index and
 * the id will *always* be valid.
 */
static inline int
tree_index_is_valid (const int id)
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

static inline int
tree_list_size ()
{
    int ret = 0;
    if (tree_read() != 0)
        goto exit;
    ret = global_tree->list_size;
    tree_unlock();
exit:
    return ret;
}

static inline int
node_write (const int id)
{
    if (!tree_index_is_valid(id))
        return 1;
    return pthread_rwlock_wrlock(&global_tree->list[id].rw_lock);
}

static inline int
node_read (const int id)
{
    if (!tree_index_is_valid(id))
        return 1;
    return pthread_rwlock_rdlock(&global_tree->list[id].rw_lock);
}

static inline int
node_unlock (const int id)
{
    return pthread_rwlock_unlock(&global_tree->list[id].rw_lock);
}

static inline int
node_data_lock (const int id)
{
    return pthread_mutex_lock(&global_tree->list[id].data_lock);
}

static inline int
node_data_trylock (const int id)
{
    return pthread_mutex_trylock(&global_tree->list[id].data_lock);
}

static inline int
node_data_unlock (const int id)
{
    return pthread_mutex_unlock(&global_tree->list[id].data_lock);
}

/*
 * Must be called with the write lock (structure) *and* the mutex lock (data)
 * acquired for the node.  Attach (and unbench) the node to the tree system and
 * give it a reference to hold.
 */
static inline void
node_mark_attached_fullwr (const int id, void *data, const int thread_id)
{
    global_tree->list[id].attached = 1;
    global_tree->list[id].benched = 0;
    global_tree->list[id].data = data;
    global_tree->list[id].thread_id = thread_id;
    global_tree->set_id_func(data, id);
}

/*
 * With a write lock:
 * Mark a node unusued, telling to system it is free to be cleaned-up.  The
 * contract has `data` because this is can be used as a callback function for
 * `tree_map_subtree`
 */
static void
node_mark_unused_wr (void *data, const int id)
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
static inline int
node_init_wr (const int id)
{
    int i, length = 5, ret = 1;

    node_mark_unused_wr(NULL, id);
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
static inline void
node_cleanup_fullwr (const int id)
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
static inline void
node_destroy_fullwr (const int id)
{
    node_cleanup_fullwr(id);

    if (global_tree->list[id].children) {
        free(global_tree->list[id].children);
        global_tree->list[id].children = NULL;
    }
}

/*
 * With a write lock:
 * Mark a node as benched. This keeps its reference id valid and its data from
 * being cleaned-up even if it removed from the tree. The contract has `data`
 * because this is can be used as a callback function for `tree_map_subtree`
 */
static void
node_mark_benched_wr (void *data, const int id)
{
    global_tree->list[id].attached = 0;
    global_tree->list[id].benched = 1;
}

/*
 * With read or write lock:
 * Returns 1 (true) or 0 (false) if the node is being used or not.
 */
static inline int
node_is_used_rd (const int id)
{
    return (global_tree->list[id].attached || global_tree->list[id].benched);
}

/*
 * Delete a node. This doesn't unlink the node from the tree nor does it check
 * if the node is used or has data.
 * Returns 0 if successful, !0 if there was an error with either of the locks.
 */
static inline int
node_delete (const int id)
{
    int ret = TREE_ERROR;

    if (node_data_lock(id) != 0)
        goto exit;

    if (node_write(id) != 0) {
        node_data_unlock(id);
        goto exit;
    }

    node_mark_unused_wr(NULL, id);
    node_cleanup_fullwr(id);
    node_unlock(id);
    node_data_unlock(id);

    ret = 0;
exit:
    return ret;
}

/*
 * Acquire the write lock on the given Node to add the given child. Doesn't
 * check the child id. 
 * If it successfully adds the child, returns 0.
 * Returns NODE_ERROR if the parent node isn't being used.
 * Returns TREE_ERROR if realloc failed.
 */
static int
node_add_child (const int id, const int child)
{
    void *memory = NULL;
    int child_id, max, ret = NODE_ERROR;

    if (node_write(id) != 0)
        goto exit;

    if (!node_is_used_rd(id))
        goto unlock;

    max = global_tree->list[id].max_children;

find_open_slot:
    for (child_id = 0; child_id < max; child_id++) {
        if (global_tree->list[id].children[child_id] == NODE_INVALID) {
            global_tree->list[id].children[child_id] = child;

            if (child_id == global_tree->list[id].last_child)
                global_tree->list[id].last_child++;

            ret = 0;
            goto unlock;
        }
    }

    /* if there's no more room for children and we haven't been here before */
    if (memory == NULL && child_id == max) {
        max *= 2;
        memory = realloc(global_tree->list[id].children, max * sizeof(Node));

        if (memory == NULL) {
            ret = TREE_ERROR;
            goto unlock;
        }

        global_tree->list[id].children = memory;
        global_tree->list[id].max_children = max;
        
        /* TODO: clamp memory size so it can't go above max actors */

        for (; child_id < max; child_id++)
            global_tree->list[id].children[child_id] = NODE_INVALID;

        goto find_open_slot;
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
static int
node_remove_child (const int id, const int child)
{
    int child_id, max_id, ret = 1;

    if (node_write(id) != 0)
        goto exit;

    max_id = global_tree->list[id].max_children;

    for (child_id = 0; child_id < max_id; child_id++) {
        if (global_tree->list[id].children[child_id] == child) {
            global_tree->list[id].children[child_id] = NODE_INVALID;

            if (global_tree->list[id].last_child == child_id)
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
static int
node_cleanup (const int id)
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
 * Initialze the tree with the given length for its node array. `set_id` is for
 * assigning the Node's id to the data that it holds. `cleanup` is what the
 * tree uses for its garbage collection.
 * Returns 0 if no errors.
 */
int
tree_init (int length, data_set_id_func_t set_id, data_cleanup_func_t cleanup)
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
 * Returns NODE_INVALID if realloc failed when adding a child.
 *
 * Returns NODE_ERROR if parent_id > -1 *and* the parent_id isn't in use.
 *
 * Returns TREE_ERROR
 *      - there are no more unused nodes
 *      - write-lock fails while setting the root node
 */
int
tree_add_reference (void *data, int parent_id, const int thread_id)
{
    int max_id, id, ret = TREE_ERROR;

    if (data == NULL)
        goto exit;

find_unused_node:
    max_id = tree_list_size();

    /* find first unused node and clean it up if needed */
    for (id = 0; id < max_id; id++)
        if (id != parent_id && node_cleanup(id) == 0)
            goto data_lock_and_write;

    /* if we're here, no unused node was found */
    global_tree->cleanup_func(data);
    goto exit;

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
    global_tree->list[id].parent = parent_id;

    node_unlock(id);
    node_data_unlock(id);

    /*
     * The first time no valid parent_id is passed, the node becomes the root
     * of the tree. After that, the node is created as the child of the root.
     */
    if (parent_id <= NODE_INVALID) {
        if (tree_write() != 0) {
            ret = TREE_ERROR;
            goto delete_node;
        }

        if (global_tree->root == NODE_INVALID) {
            global_tree->root = id;
            tree_unlock();
            ret = id;
            goto exit;
        }

        parent_id = global_tree->root;
        tree_unlock();
    }

    /*
     * `id` could potentially be invalid at the time we add it to the parent
     * because we have unlocked it. Invalid ids are checked, thus it won't be a
     * problem if a node has an invalid id as one of its children.  When the
     * system encounters (in tree_map_subtree) an invalid id, it should remove
     * it.
     */
    if (node_add_child(parent_id, id) != 0) {
        ret = NODE_ERROR;
delete_node:
        if (node_delete(id) != 0)
            ret = TREE_ERROR;
        goto exit;
    }

    ret = id;
exit:
    return ret;
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
tree_unlink_reference (const int id, const int is_delete)
{
    int ret = 1;
    void (*unlink_func)(void*, int);

    if (node_write(id) != 0)
        goto exit;

    if (!node_is_used_rd(id))
        goto unlock;

    if (is_delete)
        unlink_func = node_mark_unused_wr;
    else
        unlink_func = node_mark_benched_wr;

    /* if we're deleting the root node, set the root node to invalid */
    if (global_tree->list[id].parent == NODE_INVALID && tree_root() == id) {
        if (tree_write() == 0) {
            global_tree->root = NODE_INVALID;
            tree_unlock();
            ret = 0;
        }
        goto unlock;
    }

    /* if the node isn't benched and has an invalid parent id */
    if (!global_tree->list[id].benched
        && node_remove_child(global_tree->list[id].parent, id) != 0) {
        goto unlock;
    }

    ret = 0;
unlock:
    node_unlock(id);

    if (ret == 0)
        tree_map_subtree(id, unlink_func, NULL, TREE_WRITE, TREE_RECURSE);

exit:
    return ret;
}

/*
 * Re-link a (benched) reference back into the tree. If parent is > -1 the
 * node rejoined to the tree as a child of that parent.
 * Returns 0 if successful.
 * Returns TREE_ERROR if the there was an error re-linking back to its parent.
 * Returns NODE_ERROR if the node wasn't benched.
 * Returns NODE_INVALID if the node was invalid.
 */
int
tree_link_reference (const int id, const int parent)
{
    int parent_id = NODE_INVALID;
    int ret = NODE_INVALID;

    if (node_write(id) != 0)
        goto exit;

    if (!node_is_used_rd(id)) {
        node_unlock(id);
        goto exit;
    }

    if (global_tree->list[id].attached) {
        ret = NODE_ERROR;
        node_unlock(id);
        goto exit;
    }

    if (parent > NODE_INVALID)
        global_tree->list[id].parent = parent;

    parent_id = global_tree->list[id].parent;
    global_tree->list[id].attached = 1;
    global_tree->list[id].benched = 0;

    node_unlock(id);

    if (node_add_child(parent_id, id) != 0) {
        ret = TREE_ERROR;

        if (node_write(id) != 0)
            goto exit;
        node_mark_benched_wr(NULL, id);
        node_unlock(id);
        goto exit;
    }

    ret = 0;
exit:
    return ret;
}

/*
 * Get the data (pointer) referenced by the id.
 *
 * Returns NULL if the id is invalid.  Calling `tree_deref` isn't necessary
 * (and is undefined) if this function returns NULL.
 *
 */
void *
tree_ref (const int id)
{
    void *data = NULL;

    if (node_data_lock(id) != 0)
        goto exit;

    if (global_tree->list[id].data == NULL) {
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
tree_deref (const int id)
{
    return node_data_unlock(id);
}

/* 
 * Explicitly garbage collect the node at id. 
 * Returns NODE_ERROR if the node *isn't* garbage!
 * Returns TREE_ERROR if any locks fail to acquire.
 * Returns 0 if successful.
 */
int
tree_node_cleanup (const int id)
{
    int ret = TREE_ERROR;

    if (node_data_lock(id) != 0)
        goto exit;

    if (node_write(id) != 0)
        goto unlock_data;

    if (node_is_used_rd(id)) {
        ret = NODE_ERROR;
        goto unlock;
    }

    node_cleanup_fullwr(id);
    ret = 0;
unlock:
    node_unlock(id);
unlock_data:
    node_data_unlock(id);
exit:
    return ret;
}

/*
 * Map the given callback function to the subtree starting at the given root
 * node. The root node can be any node in the tree.
 *
 * If is_read is true, then read locks will be acquired before calling the
 * callback. Otherwise write locks will be used.
 *
 * If is_recurse is true, the callback will be called recursively starting at
 * the root node and end when every ancestor of the that node has been
 * processed.  If is_recurse is false then the function will only do the given
 * root and its direct children.
 *
 * The data is any data that needs to be passed into the callback. The callback
 * function is always passed the current Node's id.
 */
int
tree_map_subtree (const int root,
        const map_callback_t function,
        void *data,
        const int is_read,
        const int is_recurse)
{
    int (*lock_func)(int);
    int id, cid;

    if (is_read)
        lock_func = node_read;
    else
        lock_func = node_write;

    if (lock_func(root) != 0)
        return NODE_INVALID;

    if (!node_is_used_rd(root)) {
    	node_unlock(root);
        return NODE_INVALID;
    }

    /* 
     * use c99's VLA and `in-function` stack variable declaration because a 
     * node's children count is not equal across all nodes.
     */
    const int max_children = global_tree->list[root].last_child;
    int children[max_children];

    memset(&children, -1, sizeof(int) * max_children);
    function(data, root);

    for (cid = 0, id = 0; id < max_children; id++) {
        if (global_tree->list[root].children[id] == NODE_INVALID)
            continue;

        children[cid] = global_tree->list[root].children[id];
        cid++;
    }

    /* 
     * we've got the list of children & did the function, no compelling reason
     * to hold this lock. this means the ids in `children` *could* become 
     * invalid by the time they are used, but the system is built to handle 
     * invalid ids, so its worth it for the extra time without a lock held.
     */
    node_unlock(root);
    
    if (is_recurse) {
        for (id = 0; id < cid; id++) {
            if (tree_map_subtree(children[id], function, 
                        data, is_read, is_recurse) != 0) {
                // unlink children[id] from root
            }
        }
    } else {
        for (id = 0; id < cid; id++) {
            if (lock_func(children[id]) != 0)
                continue;

            if (!node_is_used_rd(children[id])) {
                node_unlock(children[id]);
                // unlink children[id] from root
                continue;
            }

            function(data, children[id]);
            node_unlock(children[id]);
        }
    }

    return 0;
}

/*
 * Returns the thread id for the node of the given id.
 * Returns NODE_ERROR if the node at id is garbage.
 * Returns TREE_ERROR if the node read lock fails.
 */
int
tree_node_thread (const int id)
{
    int ret = TREE_ERROR;

    if (node_read(id) != 0)
        goto exit;

    if (!node_is_used_rd(id)) {
        ret = NODE_ERROR;
        goto unlock;
    }

    ret = global_tree->list[id].thread_id;
unlock:
    node_unlock(id);
exit:
    return ret;
}

/*
 * Returns the parent of the node (NODE_INVALID is a valid return).
 * Returns NODE_ERROR if the node at id is garbage.
 * Returns TREE_ERROR if the node read lock fails.
 */
int
tree_node_parent (const int id)
{
    int ret = TREE_ERROR;

    if (node_read(id) != 0)
        goto exit;

    if (!node_is_used_rd(id)) {
        ret = NODE_ERROR;
        goto unlock;
    }

    ret = global_tree->list[id].parent;
unlock:
    node_unlock(id);
exit:
    return ret;
}

/*
 * Returns the id of the root node. Returns NODE_ERROR if an error occurs.
 */
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

/*
 * Mark all active Nodes as garbage and clean them up. Then free the memory for
 * the Tree and the list of Nodes.
 */
void
tree_cleanup ()
{
    int id, max_id = tree_list_size();

    for (id = 0; id < max_id; id++) {
        /* Better to cause an memory leak than to lock up */
        if (node_data_trylock(id) != 0)
            continue;
        node_write(id);
        node_destroy_fullwr(id);
        node_unlock(id);
        node_data_unlock(id);
    }

    free(global_tree->list);
    free(global_tree);
}
