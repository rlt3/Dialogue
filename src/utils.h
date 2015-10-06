#ifndef DIALOGUE_UTILS
#define DIALOGUE_UTILS

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/*
 * Push a pointer and associate a metatable with it.
 */
void
lua_object_push (lua_State *L, void *object_ptr, const char *metatable);

/*
 * Push an object's method and also the object to reference `self`
 */
void
lua_method_push (lua_State *L, 
        void *object_ptr, 
        const char *metatable, 
        const char *method);

/*
 * Push the first element of a table at index.
 */
void
table_push_head (lua_State *L, int index);

/*
 * Push N elements after first of a table at index. Returns elements pushed.
 */
int
table_push_data (lua_State *L, int index);

/*
 * Expects a table at the top of the 'from' stack. Pushes table onto 'to' stack.
 */
void
table_push_copy (lua_State *from, lua_State *to, int index);

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
