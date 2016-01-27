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

void
list_read ()
{
    pthread_rwlock_rdlock(&list_rw_lock);
}

void
list_write ()
{
    pthread_rwlock_wrlock(&list_rw_lock);
}

void
list_unlock ()
{
    pthread_rwlock_unlock(&list_rw_lock);
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
    return pthread_rwlock_wrlock(&list[id].rw_lock);
}

int 
node_read (int id)
{
    if (!id_valid_index(id))
        return 1;
    return pthread_rwlock_rdlock(&list[id].rw_lock);
}

int 
node_unlock (int id)
{
    return pthread_rwlock_unlock(&list[id].rw_lock);
}

/*
 * Returns 1 (true) or 0 (false) if the node is being used or not.
 * Can return -1 (error) if the id isn't valid.
 */
int
node_is_used (int id)
{
    int ret = -1;

    if (!id_valid_index(id))
        goto exit;

    node_read(id);
    ret = (list[id].attached || list[id].benched);
    node_unlock(id);

exit:
    return ret;
}

/*
 * Find an `unused' Node, set it to be `used', and return its id.
 */
int
find_free_node ()
{
    int id, max_id;

find_unused_node:
    list_read();
    max_id = list_size;
    list_unlock();

    /* TODO: fail state where no free node is found */
    for (id = 0; id < max_id; id++)
        if (!node_is_used(id))
            goto acquire_write_lock;

acquire_write_lock:
    node_write(id);

    /*
     * The id we found could theoretically be `found' by another thread, so
     * after acquiring the write-lock on it, we double-check it is unused or we
     * loop back to find another unused one.
     */
    if (node_is_used(id))
        goto find_unused_node;

    list[id].attached = 1;
    node_unlock(id);

    return id;
}

void
node_init (id)
{
    list[id].attached = 0;
    list[id].benched = 0;
    pthread_rwlock_init(&list[id].rw_lock, NULL);
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
        node_init(i);

    printf("%d\n", find_free_node());
    printf("%d\n", find_free_node());
    printf("%d\n", find_free_node());
    printf("%d\n", find_free_node());
    printf("%d\n", find_free_node());

    status_code = 0;
    free(list);
exit:
    return status_code;
}
