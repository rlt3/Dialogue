#ifndef DIALOGUE
#define DIALOGUE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/*
 * Create the Dialogue table with all the Actions in the given Lua state.
 */
void
create_dialogue_table (lua_State *L);

/*
 * Create the Dialogue table using above, but add the garbage collection
 * function to the metatable.
 */
int
luaopen_Dialogue (lua_State *L);

#endif
