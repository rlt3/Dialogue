#ifndef DIALOGUE_COMPANY
#define DIALOGUE_COMPANY

/*
 * TODO:
 * The Company is a tree of Actors. To keep tree operations thread-safe and to 
 * shoot for as little of lock-time possible, we have a specific setup.
 *
 * Individual nodes of the tree are kept in an array. The array index is that
 * node's id (and consequently, the Actor's too, who is occupying that node). 
 * The Nodes *are* part of the tree, but can be free from the tree too. So,
 * each Node has a state of attached or detached.
 *
 * We keep a count of the total number of actors under a mutex. Each node is
 * protected by a read/write lock. The read/write lock keeps throughput (reads)
 * of the structure flowing (reads from multiple threads) until it changes. And
 * since each Node has its own read/write lock, we can have parts of the tree
 * readable while others are locked.
 *
 * If we remove any node, its reference can be updated throughout the tree as
 * the errors occur. So, when a `next_sibling' reference is pulled up, we can
 * automate it be set to -1 (our code for nil) when its references inevitably
 * returns as bad (which it will if a Node exists, yet is not attached).
 *
 * The system automatically filters itself of bad references and errors out
 * gracefully for anything else that uses bad references too.
 *
 * (To arbitrarily remove a Node, I need to lock both the Actor mutex and the
 * write lock on the Node itself. While references can be handled gracefully,
 * we cannot free memory which another thread might be currently reading.)
 */

#include "actor.h"

typedef struct Company Company;

/*
 * Create a new Company structure. The buffer_length is how many elements the
 * array is first created with and also how many elements are appended to the 
 * array if it needs to be resized (created more Actors than the buffer length).
 */
Company *
company_create (int buffer_length);

/*
 * Acquire the write lock on the Company list and cleanup any existing nodes
 * (and the Node's corresponding Actor). Then free memory for the Company list
 * and Company itself.
 */
void
company_close (Company *company);

/*
 * Acquire the write lock on the Company list. Find a Node in the list which
 * isn't being used. The index of that node becomes the id of the Actor. Create
 * that Actor.
 *
 * If parent_id is greater than 0, add the created Actor as a child of the 
 * parent Actor whose id is parent_id.
 *
 * Return the id of the created Actor. Returns -1 if an error occurs.
 */
int
company_add_actor (Company *company, lua_State *L, int parent_id);

/*
 * Verify given Actor is in Company and a valid pointer.  Returns -1 if Actor
 * pointer given is NULL or Actor doesn't belong to company.
 */
int
company_ref_actor (Company *company, Actor *actor);

/*
 * Verify id is a valid id for company. Returns the Actor pointer that
 * corresponds to the given id.  Returns NULL if the Actor doesn't exist for
 * the given id or id isn't a valid index.
 */
Actor *
company_deref_actor (Company *company, int id);

#endif
