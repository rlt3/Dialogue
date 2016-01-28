#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree.h"

typedef struct Node {
    void *data;
    int attached;
    int benched;
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
 * shrinks, all we need to do is check that `id' is >= 0 and <= max index and
 * the id will *always* be valid.
 */
int
tree_index_is_valid (int id)
{
    int ret = 0;

    if (id < 0)
        goto exit;

    tree_read();
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
node_attach_wr (int id, void *data)
{
    global_tree->list[id].attached = 1;
    global_tree->list[id].benched = 0;
    global_tree->list[id].data = data;
}

/*
 * Acquires the write-lock for the list. Resizes the list by the factor given
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

    node_attach_wr(id, data);
    node_unlock(id);

exit:
    return id;
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

void
node_cleanup (int id)
{
    global_tree->cleanup_func(global_tree->list[id].data);
    pthread_rwlock_destroy(&global_tree->list[id].rw_lock);
}

void
tree_cleanup ()
{
    int id;

    if (tree_write() != 0)
        goto free;

    for (id = 0; id < global_tree->list_size; id++)
        node_cleanup(id);

    tree_unlock();
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

    for (i = 0; i < 21; i++)
        printf("%d\n", tree_add_reference(NULL));

    status = 0;
    tree_cleanup();
exit:
    return status;
}
