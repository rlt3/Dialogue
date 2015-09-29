#include "utils.h"

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
