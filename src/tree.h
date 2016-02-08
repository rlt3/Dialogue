#ifndef DIALOGUE_TREE
#define DIALOGUE_TREE

/*
 * The Tree represents the structure of our Actors. This means it takes on the
 * responsibility of the Actor's lifetime, the structure of Actors and the 
 * functions associated (creating, moving, deleting...), and the mutual 
 * exclusive properties needed for all of that to work in parallel.
 *
 * When starting the system, the tree is initialized with the garbage collection 
 * function it needs for the particular data it is holding.  In our case the
 * Tree holds Actor data.  The Tree garbage collects data when given new data
 * to hold. So, if there is some garbage data that is unused, it is cleaned up
 * just before it is used for the new data.
 *
 * The actual data itself is protected via a mutex lock and can only be 
 * 'checked-out' by one thread at a time. The structure of the data (where it 
 * sits in the tree -- it's parent and children) is protected under a read/write
 * lock. 
 *
 * Since the data is separated from its structure, the data can be checked-out
 * and used (even changed) while the structure is changing. Such changes may be
 * deletion, benching (remove it from the tree without removing it from the 
 * system), or even adding new children. 
 *
 * This type of structure allows for many parts of the system premium
 * read-access over the tree will minimizing the number of mutually exclusive 
 * operations.
 *
 * The Tree allows for all of the fancy operations to occur by giving each
 * piece of data given to the Tree an id. The id may be invalid at any given
 * moment, but because it is an integer and not an allocated data type, we 
 * don't get a memory-leak. The ability to work on data that may be invalid at
 * any moment without negative consequences gives us power.
 *
 * Because all references to the data just ids (ints), then we can arbitrarily
 * remove nodes of the Tree, invalidating many ids. We can also reference any
 * particular piece of data through system runs (if every time the system is
 * ran it uses the same Tree structure).
 *
 * But most importantly, it allows us to send off an id saying, "I want the 
 * piece of data this id represents to be put inside this function." And since
 * ids aren't memory addresses, we don't have to worry about any segfaults or
 * memory leaks occuring down the line -- if the id isn't valid anymore, skip
 * it.
 */

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
tree_add_reference (void *data, int parent_id, int thread_id);

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
 * Re-link a (benched) reference back into the tree. 
 * Returns 0 if successful.
 * Returns 1 if the there was an error re-linking back to its parent.
 * Returns 2 if the node wasn't benched.
 * Returns 3 if the node was invalid.
 */
int
tree_link_reference (int id);

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
 * Returns the thread id for the node of the given id.
 * Returns NODE_ERROR if an error occurs (bad node, etc)
 */
int
tree_reference_thread (int id);

/*
 * Mark all active Nodes as garbage and clean them up. Then free the memory for
 * the Tree and the list of Nodes.
 */
void
tree_cleanup ();

#endif
