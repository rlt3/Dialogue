#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

//cc -Wall -std=c99 -pedantic -fPIC -Isrc/ -I./ -I/usr/include/lua5.2/ -shared -o Script.so test.c

int
lua_script_new (lua_State *L)
{
    const int args = lua_gettop(L);
    const char *meta_string = lua_tostring(L, lua_upvalueindex(1));

    lua_pushvalue(L, lua_upvalueindex(2));

    if (lua_isnil(L, -1) || !lua_isfunction(L, -1)) {
        lua_newtable(L);
        goto set_meta;
    }

    lua_call(L, args, 1);
    luaL_checktype(L, -1, LUA_TTABLE);

set_meta:
    luaL_getmetatable(L, meta_string);
    lua_setmetatable(L, -2);

    return 1;
}

int
lua_script (lua_State *L)
{
    const char *script_name = NULL;
    const int script_arg = 1;
    const int func_arg = 2;

    luaL_checktype(L, script_arg, LUA_TSTRING);
    luaL_checktype(L, func_arg, LUA_TFUNCTION);

    script_name = lua_tostring(L, script_arg);

    luaL_newmetatable(L, script_name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    lua_pushvalue(L, script_arg);
    lua_pushvalue(L, func_arg);
    lua_pushcclosure(L, lua_script_new, 2);
    lua_setfield(L, -2, "new");

    lua_setglobal(L, script_name);

    return 0;
}

int
luaopen_Script (lua_State *L)
{
    lua_pushcfunction(L, lua_script);
    lua_setglobal(L, "Script");
    return 0;
}
