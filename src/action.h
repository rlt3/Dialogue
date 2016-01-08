#ifndef DIALOGUE_ACTION
#define DIALOGUE_ACTION

#include "dialogue.h"

/*
 * These are the Actions of Dialogue -- all the primitives of the system.
 */

int
lua_action_create (lua_State *L);

int
lua_action_bench (lua_State *L);

int
lua_action_join (lua_State *L);

int
lua_action_deliver (lua_State *L);

int
lua_action_load (lua_State *L);

int
lua_action_error (lua_State *L);

#endif
