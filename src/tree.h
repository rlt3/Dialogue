#ifndef DIALOGUE_TREE
#define DIALOGUE_TREE

typedef void (*data_set_id_func_t) (void *, int);
typedef void (*data_cleanup_func_t) (void *);
typedef int (*data_lookup_func_t) (void *);

enum TreeReturn {
    ERROR = -3,
    NODE_ERROR = -2,
    NODE_INVALID = -1
};

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
tree_add_reference (void *data, int parent_id);

/*
 * Unlink a Node and all of its descendents from the tree. If is_delete is 1,
 * this will mark all the nodes unlinked as garbage so they can be cleaned-up 
 * and used by the system.
 *
 * If is_delete is 0 then the nodes are benched and aren't attached to the tree
 * but are otherwise still around and are not marked as garbage.
 */
int
tree_unlink_reference (int id, int is_delete);

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
tree_ref (int id);

/*
 * Free up the data for some other process. If `tree_ref' returned NULL,
 * calling this function produces undefined behavior.
 */
int
tree_deref (int id);

/*
 * Mark all active Nodes as garbage and clean them up. Then free the memory for
 * the Tree and the list of Nodes.
 */
void
tree_cleanup ();

#endif
