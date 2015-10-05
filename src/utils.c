#include "utils.h"

/*
 * Push a pointer and associate a metatable with it.
 */
void
lua_object_push (lua_State *L, void *object_ptr, const char *metatable)
{
    lua_pushlightuserdata(L, object_ptr);
    luaL_getmetatable(L, metatable);
    lua_setmetatable(L, -2);
}

/*
 * Expects a table at the top of the 'from' stack. Pushes table onto 'to' stack.
 */
void
lua_table_copy (lua_State *from, lua_State *to)
{
    int i;
    lua_newtable(to);
    lua_pushnil(from);
    for (i = 1; lua_next(from, -2); i++) {
        lua_pushstring(to, lua_tostring(from, -1));
        lua_rawseti(to, -2, i);
        lua_pop(from, 1);
    }
}

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
        lua_CFunction function)
{
    /* create metatable */
    luaL_newmetatable(L, metatable);

    /* metatable.__index = metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    /* register methods */
    luaL_setfuncs(L, methods, 0);

    /* Object() => new Object */
    lua_pushcfunction(L, function);

    return 1;
}
