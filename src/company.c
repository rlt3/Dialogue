#include <stdlib.h>
#include "tree.h"
#include "company.h"

/*
 * Make, Lookup, Set Id, and Remove. Allocate some data and free it. Used to
 * test we don't have memory leaks.
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

/*
 * Create the Company tree with the following options.
 */
int
company_create (int base_actors, int max_actors, int base_children)
{
    return tree_init(base_actors, max_actors, 2, set, rm, lk);
}

int 
company_add (int parent)
{
    return tree_add_reference(mk(0), parent);
}

int
company_remove (int id)
{
    return tree_unlink_reference(id, 1);
}

void *
company_ref (int id)
{
    return tree_ref(id);
}

int
company_deref (int id)
{
    return tree_deref(id);
}

void
company_close ()
{
    tree_cleanup();
}
