#ifndef DIALOGUE_UTILS
#define DIALOGUE_UTILS

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/*
 * Set an array (table) at index to given string. Expects table at top of stack.
 * array[index] = string
 */
void
lua_array_set_index (lua_State *L, int index, const char *string);

/*
 * To be used with luaopen_ModuleName.
 *
 * Creates a new metatable, registers methods, and returns a function to 
 * create objects of the metatable type.
 */
int 
lua_meta_open (lua_State *L, 
        const char *metatable, 
        const luaL_Reg *methods, 
        lua_CFunction function);

#endif
