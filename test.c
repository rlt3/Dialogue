#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int
emit (lua_State *L)
{
    const int table = 1;
    luaL_checktype(L, table, LUA_TTABLE);

    lua_pushnil(L);
    while (lua_next(L, table)) {
        printf("%s\n", lua_tostring(L, -1));
        //printf("%s -> %s\n", 
        //        lua_tostring(L, -2),
        //        lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    
    return 0;
}

int
create (lua_State *L)
{
    const int args = lua_gettop(L);
    const int table = args + 1;
    int k, i;
    
    if (args & 1)
        luaL_error(L, "Need an even number of arguments!");

    lua_newtable(L);

    for (k = 0, i = 1; k < args; k += 2, i++) {
        lua_pushvalue(L, k + 1);
        lua_pushvalue(L, k + 2);

        if (lua_isnil(L, -2)) {
            /* if no key exists, set value as array and pop nil key */
            lua_rawseti(L, table, i);
            lua_pop(L, 1);
        } else {
            lua_settable(L, table);
        }
    }

    //for (k = args, i = 1; k > 0; k -= 2, i++) {
    //    table = k - 1;
    //    lua_insert(L, table);

    //}

    return 1;
}

//clang -o Script.so -Wall -std=c99 -pedantic -fPIC -shared -I/usr/include/lua5.2/ test.c -llua5.2
//cc -Wall -std=c99 -pedantic -D _BSD_SOURCE -fPIC -bundle -undefined dynamic_lookup -Isrc/ -I./ -I/usr/local/include/ -o Script.so test.c -L./ -L/usr/local/lib -llua

int
luaopen_Script (lua_State *L)
{
    lua_pushcfunction(L, emit);
    lua_setglobal(L, "e");

    lua_pushcfunction(L, create);
    lua_setglobal(L, "c");

    return 0;
}
