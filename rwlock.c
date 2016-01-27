#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct Node {
    int attached;
    int benched;
    pthread_rwlock_t rw_lock;
} Node;

/* rwlock protects the list and list_size */
static pthread_rwlock_t list_rw_lock;
static int list_size = 10;
static Node *list;

int
list_read ()
{
    return pthread_rwlock_rdlock(&list_rw_lock);
}

int
list_write ()
{
    return pthread_rwlock_wrlock(&list_rw_lock);
}

int
list_unlock ()
{
    return pthread_rwlock_unlock(&list_rw_lock);
}

/*
 * Return 1 (true) or 0 (false) if the id is a valid index or not.
 */
int
id_valid_index (int id)
{
    int ret = 0;

    if (id < 0)
        goto exit;

    list_read();
    ret = !(id > list_size);
    list_unlock();

exit:
    return ret;
}

int 
node_write (int id)
{
    if (!id_valid_index(id))
        return 1;
    //printf("Write %d\n", id);
    return pthread_rwlock_wrlock(&list[id].rw_lock);
}

int 
node_read (int id)
{
    if (!id_valid_index(id))
        return 1;
    //printf("Read %d\n", id);
    return pthread_rwlock_rdlock(&list[id].rw_lock);
}

int 
node_unlock (int id)
{
    //printf("Unlock %d\n", id);
    return pthread_rwlock_unlock(&list[id].rw_lock);
}

/*
 * With read lock:
 * Returns 1 (true) or 0 (false) if the node is being used or not.
 */
int
node_is_used_rd (int id)
{
    return (list[id].attached || list[id].benched);
}

void
node_init_wr (int id)
{
    list[id].attached = 0;
    list[id].benched = 0;
    pthread_rwlock_init(&list[id].rw_lock, NULL);
}

/*
 * Acquires the write-lock for the list. Resizes the list to double its current 
 * size. Returns 1 if successful, 0 otherwise.
 */
int
double_list_size ()
{
    const int factor = 2;
    Node *array = NULL;
    int id, ret = 0;

    list_write();

    id = list_size;
    array = realloc(list, (list_size * factor) * sizeof(Node));

    if (array != NULL) {
        ret = 1;
        list = array;
        list_size *= factor;
        for (; id < list_size; id++)
            node_init_wr(id);
    }

    list_unlock();

    return ret;
}

/*
 * Find an `unused' Node, set it to be `used', and return its id.  Returns -1
 * if no unused Node was found *and* the attempt to resize the Node list (to
 * create more unused nodes) failed.
 */
int
find_free_node ()
{
    int max_id, id = 0;

find_unused_node:
    list_read();
    max_id = list_size;
    list_unlock();

    /* Find the first unused node and then jump to get the write-lock on it */
    for (; id < max_id; id++) {
        if (node_read(id) == 0) {
            if (!node_is_used_rd(id))
                goto unlock_and_write;
            node_unlock(id);
        }
    }

    /* if we're here, no unused node was found */
    if (!double_list_size()) {
        id = -1;
        goto exit;
    } else {
        goto find_unused_node;
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

    list[id].attached = 1;
    node_unlock(id);

exit:
    return id;
}

int 
main (int argc, char **argv)
{
    int i, status_code = 1;

    list = malloc(sizeof(Node) * list_size);
    if (!list)
        goto exit;

    pthread_rwlock_init(&list_rw_lock, NULL);
    for (i = 0; i < list_size; i++)
        node_init_wr(i);

    for (i = 0; i < 15; i++)
        printf("%d\n", find_free_node());

    status_code = 0;
    free(list);
exit:
    return status_code;
}
