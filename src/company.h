/*============================================================================/
 
    The Company represents the Tree of Actors. All Actors are created through
  the Company either through Lua or through its public C functions.

  The Lua aspect is actually the user facing part of the code and so acts as a
  wrapper around the functions below. The Company is represented as a table
  called 'Actor' in Lua. The table has a __call metamethod which returns Actor
  reference objects. (I chose "Actor" as the name in Lua for the Company
  because it essentially spits out actors like a constructor might.)

  The __call metamethod can create new references, return old references, or
  even return invalid references. Actor objects in Lua are merely tables with
  the 'Actor' metatable attached and an integer id inside. They are this way
  because the Company uses Tree.h to handle the Actors' memory and thread-safety
  and Tree.h operates off integer ids.

  The Company essentially combines the two aspects of the Actors: its placement
  inside the Dialogue, where it is in the Tree, and its data, the Lua state of
  each Actor. For example, actor methods like "bench", "join", and "children"
  all are about the structure (whether it's in the tree or not, its current
  children in the tree, etc) while methods like "send", "load", and "probe" are
  all about the Lua state of the Actor.
  
/============================================================================*/

#ifndef DIALOGUE_COMPANY
#define DIALOGUE_COMPANY

#include "actor.h"

/*
 * Create the Company tree with the following options.
 *
 * base_actors is the default number of actors that can be created before the
 * tree needs to resize for more.
 *
 * max_actors is the maximum size the actor list can be even after resizing.
 *
 * base_children is the default number of children an actor can have before
 * resizing.
 */
int
company_create (int base_actors, int max_actors, int base_children);

/*
 * Set the Company's table inside the given Lua state.
 */
void
company_set (lua_State *L);

/*
 * An actor can be represented in many ways. All of them boil down to an id.
 * This function returns the id of an actor at index. Will call lua_error on
 * L if the type is unexpected.
 */
int
company_actor_id (lua_State *L, int index);

/*
 * Push an Actor reference object onto the Lua stack.
 */
void
company_push_actor (lua_State *L, int actor_id);

/*
 * Pushes a table of actor ids which correspond to the audience of the actor by
 * the tone.
 */
void
company_push_audience (lua_State *L, int id, const char *tone);

/*
 * Add an Actor to the Company. Expects an Actor's definition table on top of
 * L. If thread_id > -1 then the created Actor will only be ran on the thread
 * with that id (thread_id == 0 is the main thread).  Returns the id of the new
 * Actor if successful otherwise it will call lua_error on L. 
 */
int 
company_add (lua_State *L, int parent, int thread_id);

/*
 * Remove an actor from the Company's Tree but still leave it accessible in
 * memory (to be reloaded or otherwise tested). Will error through L if the
 * id is invalid.
 */
void 
company_bench (lua_State *L, int id);

/*
 * Join an actor which was benched back into the Company's tree. If the parent 
 * is >NODE_INVALID then the benched Actor is joined as a child of that parent.
 * Will error through L.
 */
void
company_join (lua_State *L, const int id, const int parent);

/*
 * Remove an Actor from the Company's Tree and mark it as garbage. Will error
 * through L if the id is invalid.
 */
void 
company_delete (lua_State *L, int id);

/*
 * Return the parent of the actor with id.
 * Will error through L if the actor at id isn't used.
 */
int
company_actor_parent (lua_State *L, const int id);

/*
 * Get the actor associated with the id. Will error out on L if the id isn't a
 * valid reference. Requires called `company_deref` is the return was *not*
 * NULL.
 */
Actor *
company_ref (lua_State *L, int id);

/*
 * Return the actor associated with the id. Calling this function for an id
 * that returned NULL for `company_ref` produces undefined behavior.
 */
int
company_deref (int id);

/*
 * Destroy the Company and all the Actors.
 */
void
company_close ();

#endif
