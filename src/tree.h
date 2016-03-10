/*============================================================================/
 
        A
      / | \
     B  E  F
    / \
   C   D     G

       The Tree takes ownership of data and returns an id to the Node holding
   the data inside the tree. All operations of the Node (creation, deletion, 
   etc) are done through the id.  The Nodes inside the tree only point to their
   children and to their parent and do so using ids.
   
   The Tree was written with the expectation of having the tree be manipulated
   asynchronously and in parallel. The advantage of using integer ids 
   internally and externally is that it allows flexibility under parallel and
   asynchronous environments by not giving any an explicit reference, but
   an implicit one.
   
   Another advantage of having an id is that one can reference any Node
   (assuming the same structure) through runs of the program. e.g. if `F` is id
   `6` and the Tree's structure remains the same, we can say that `6 == F` and
   vice-versa.

   There is a `map` utility for subtrees. A subtree is any Node that exists
   inside the tree.  This means that Node `A` above is techincally a subtree
   and that `B`, `E`, and `F` are as well.
   
   The utility can be recursive or only go one-level deep. For instance, the
   subtree at `A` that is only one-level deep only represents `B`, `E`, and 
   `F`. But that if the utility was called recursively, it would represent the
   entire tree.

   The Tree has the notion of attached and benched Nodes. Nodes which are 
   attached are in the tree, e.g. `E`. A benched Node is one that is in the
   system but is not attached to the tree, e.g. `G`. 
   
   The `map` utility skips benched Nodes so that operations on the data it 
   holds can occur outside the working environment of the tree. The Tree 
   doesn't garbage collect benched nodes.
   
   Let's say that `G` has a parent of `F`, if `G` unbenched (thus reattached),
   it would become the child of `F`. If for some reason `F` doesn't exist
   anymore, unbenching `G` would error and it would stay benched.

   Nodes which aren't attached nor benched are considered garbage and thus they
   are garbage collected. When Nodes are requested to be deleted they are 
   detached from the tree along with all its descendents and are marked as
   garbage. When the Tree needs to store more data it finds the first garbage
   Node, garbage collects it, and then uses it to store the data.

   All Nodes are garbage collected when the system is shutdown.

/============================================================================*/

#ifndef DIALOGUE_TREE
#define DIALOGUE_TREE

typedef void (*data_set_id_func_t) (void *, int);
typedef void (*data_cleanup_func_t) (void *);
typedef void (*map_callback_t) (void *, int);

#define TREE_WRITE       0
#define TREE_READ        1
#define TREE_RECURSE     1
#define TREE_NON_RECURSE 0
#define TREE_ERROR      -3
#define NODE_ERROR      -2
#define NODE_INVALID    -1

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
        data_cleanup_func_t cleanup);

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
void
tree_map_subtree (int root, 
        map_callback_t function, 
        void *data, 
        int is_read, 
        int is_recurse);

/*
 * Returns the id of the root node. Returns NODE_ERROR if an error occurs.
 */
int
tree_root ();

/*
 * Returns the parent id of the Node. Returns NODE_ERROR if an error occurs.
 */
int
tree_node_parent (int id);

/*
 * Mark all active Nodes as garbage and clean them up. Then free the memory for
 * the Tree and the list of Nodes.
 */
void
tree_cleanup ();

#endif
