#ifndef DIALOGUE_COMPANY
#define DIALOGUE_COMPANY

/*
 * A Company of Actors.
 *
 * The Company is the tree of Actors. A tree is used because it is the most
 * natural structure for a hierarchy. The tree also allows for easy 
 * subdivisions, e.g. the getting the audience of an Actor by a Tone.
 *
 * The internals of the Company is handled by an array of nodes. Each node 
 * holds a pointer to an Actor, its parent, next sibling, and its first child.
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

void
company_close (Company *company);

/*
 * Since the Company has all references to Actors in a single place, we don't
 * need to pass by pointer. This means we can just use the IDs of each Actor
 * (its index in the Company's array) to handle all operations. Because of this
 * we can stop worrying about metatables and userdata and lightuserdata. We can
 * have a metatable attached to a table with the Actors ID and just use that
 * for all operations.
 *
 * Because the Company handles IDs, it needs to be available (similar to the
 * Director) in the different Lua states.
 */

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
 * Verify given Actor is in Company and a valid pointer.  Increment the Actor's
 * reference count and return its id.  Returns -1 if Actor pointer given is
 * NULL or Actor doesn't belong to company.
 */
int
company_ref_actor (Company *company, Actor *actor);

/*
 * Verify id is a valid id for company. Returns the Actor pointer that 
 * corresponds to the given id and decrements its reference count. Returns NULL
 * if the Actor doesn't exist for the given id or id isn't a valid index.
 */
Actor *
company_deref_actor (Company *company, int id);

#endif
