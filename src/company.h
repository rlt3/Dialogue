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

/*
 * Create a new Company structure. The buffer_length is how many elements the
 * array is first created with and also how many elements are appended to the 
 * array if it needs to be resized (created more Actors than the buffer length).
 */
Company *
company_init (int buffer_length);

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

int
company_add_actor (lua_State *L, Company *company);

/*
 * Increment an Actor's reference count and return its id.
 */
int
company_ref_actor (Company *company, Actor *actor);

/*
 * Find an Actor by the given id and decrement its reference count. Return the
 * reference'd actor's pointer.  Returns NULL if an Actor doesn't exist with
 * the given id.
 */
Actor *
company_deref_actor (Company *company, int id);

#endif
