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
 * Add an Actor to the Company. Expects an Actor's definition table on top of
 * L. Will call lua_error on L. If thread_id > -1 then the created Actor will
 * only be ran on the thread with that id (thread_id == 0 is the main thread).
 */
int 
company_add (lua_State *L, int parent, int thread_id);

/*
 * Remove an actor from the Company's Tree but still leave it accessible in
 * memory (to be reloaded or otherwise tested).
 */
int 
company_bench (int id);

/*
 * Join an actor which was benched back into the Company's tree.
 */
int
company_join (int id);

/*
 * Remove an Actor from the Company's Tree and mark it as garbage.
 */
int 
company_delete (int id);

/*
 * Get the actor associated with the id. Returns NULL if the id wasn't valid.
 * Requires called `company_deref` is the return was *not NULL*.
 */
Actor *
company_ref (int id);

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
