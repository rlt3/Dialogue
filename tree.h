#ifndef DIALOGUE_TREE
#define DIALOGUE_TREE

typedef void (*data_set_id_func_t) (void *, int);
typedef void (*data_cleanup_func_t) (void *);
typedef int (*data_lookup_func_t) (void *);

/*
 * Initialze the tree with the given length and the cleanup function for the
 * references (pointers it owns) it holds. The `initial_length' also serves as
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
        data_lookup_func_t lookup);

/*
 * Have the tree take ownship of the pointer. The tree will cleanup that
 * pointer with the data_cleanup_func_t given in tree_init. 
 *
 * The tree attaches the pointer to a Node which is added as a child of
 * parent_id.  If parent_id <= -1 then the Node won't have a parent.
 *
 * Returns the id of the Node inside the tree. 
 *
 * Returns -1 if the tree was unable to allocate more memory for Nodes to hold
 * the reference.
 *
 * Returns -2 if parent_id > -1 *and* the parent_id is invalid.
 */
int
tree_add_reference (void *data, int parent_id);

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
tree_unlink_reference (int id, int is_delete);

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
tree_dereference (int id);

/*
 * Using the lookup_func, get the id for the Node from the data. Errors should
 * be handled through the lookup function.
 *
 * Decrements the ref_count for the Node at id. See node_cleanup.
 */
int
tree_reference (void *data);

#endif
