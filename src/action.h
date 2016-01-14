#ifndef DIALOGUE_ACTION
#define DIALOGUE_ACTION

/*
 * The Actions are the primitives of Dialogue. This means everything in the 
 * Dialogue happens through one (or many) of these Actions.
 *
 * Actions, as they are implemented in the system, are actually methods of the
 * Director metatable (where the Director could be thought of as a singleton 
 * scheduler).
 *
 * We don't put the Actions in the Director file because, while these are 
 * methods for a metatable, they aren't object methods literally belonging to
 * an object instance. The Actions belong to the system and the Director just
 * directs the Actions.
 */

#include "dialogue.h"

/*
 * Synchronous:
 * Director:new(definition_table [, parent])
 *
 * Asynchronous:
 * Director{"new", definition_table [, parent]}
 *
 * See Actor.h (specifically function `actor_create') for the definition of
 * `definition_table'.
 *
 * Create an Actor and then load it asynchronously.
 */
int
lua_action_create (lua_State *L);

int
lua_action_bench (lua_State *L);

int
lua_action_join (lua_State *L);

int
lua_action_receive (lua_State *L);

int
lua_action_send (lua_State *L);

int
lua_action_load (lua_State *L);

int
lua_action_error (lua_State *L);

#endif
