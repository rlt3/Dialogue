#ifndef DIALOGUE_TREE
#define DIALOGUE_TREE

typedef void (*data_cleanup_func_t) (void *);

/*
 * Initialze the tree with the given length and the cleanup function for the
 * references (pointers it owns) it holds. The `initial_length' also serves as
 * the base for resizing by a factor. So, if the length is 10, it is resized by
 * factors of 10.
 *
 * Returns 0 if no errors.
 */
int
tree_init (int length, int max_length, int scale_factor, data_cleanup_func_t f);

/*
 * Have the tree take ownship of the pointer. The tree will cleanup that pointer
 * with the data_cleanup_func_t given in tree_init.
 *
 * Returns the id of the reference inside the tree. Returns -1 if the tree was
 * unable to allocate more memory to hold the reference.
 */
int
tree_add_reference (void *ptr);

/*
 * Get the pointer associated with the reference id. This function doesn't pass
 * ownership. Returns NULL if the given id is invalid (either not a valid index
 * or if the id itself is invalid).
 */
void *
tree_dereference (int id);

/*
 * TODO: any reason for an inverse of tree_dereference since no pointer should
 * be getting passed around anyhow?
 */
int
tree_reference (void *ptr);

#endif
