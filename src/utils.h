#ifndef DIALOGUE_UTILS
#define DIALOGUE_UTILS

#include "dialogue.h"

static inline void utils_copy_top (lua_State *to, lua_State *from);
static inline void utils_transfer (lua_State *to, lua_State *from, const int n);

/*
 * Copy a table of the `from' stack at the index to the `to' stack.
 */
static inline void
utils_copy_table (lua_State *T, lua_State *F, const int index)
{
    //int i, len = luaL_len(from, index);
    //lua_newtable(to);

    //for (i = 1; i <= len; i++) {
    //    lua_rawgeti(from, index, i);
    //    utils_copy_top(to, from);
    //    lua_rawseti(to, -2, i);
    //    lua_pop(from, 1);
    //}

    const int table = lua_gettop(T) + 1;
    lua_newtable(T);

    int i = -1;
    while (lua_next(F, index)) {
        i++;

        /* if there *is* a key for value at -1 */
       // if (!lua_isnil(F, -2)) {
       //     lua_pushvalue(F, -2); /* push the key */
       //     /* pops pushvalue'd key & value. transfers in reverse order */
       //     utils_transfer(T, F, 2); 
       //     lua_settable(T, table);
       //     continue;
       // }

        utils_copy_top(T, F);
        lua_rawseti(T, table, i);
        lua_pop(F, 1);
    }
}

/*
 * Copy the top element of `from' and push it onto `to'. Does *not* pop the
 * `from' stack.
 */
static inline void
utils_copy_top (lua_State *to, lua_State *from)
{
    switch (lua_type(from, -1)) {
    case LUA_TNUMBER:
        lua_pushnumber(to, lua_tonumber(from, -1));
        break;
        
    case LUA_TSTRING:
        lua_pushstring(to, lua_tostring(from, -1));
        break;

    case LUA_TBOOLEAN:
        lua_pushinteger(to, lua_tointeger(from, -1));
        break;

    case LUA_TTABLE:
        utils_copy_table(to, from, lua_gettop(from));
        break;

    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
        lua_pushlightuserdata(to, lua_touserdata(from, -1));
        break;

    case LUA_TNIL:
        lua_pushnil(to);
        break;

    default:
        break;
    }
}

/*
 * Pop the top N elements of the `from' stack and push them to the `to' stack.
 */
static inline void
utils_transfer (lua_State *to, lua_State *from, const int n)
{
    int i;
    for (i = 1; i <= n; i++) {
        utils_copy_top(to, from);
        lua_pop(from, 1);
    }
}

/*
 * Push the first element of a table at index.
 */
static inline void
utils_push_table_head (lua_State *L, int index)
{
    lua_rawgeti(L, index, 1);
}

/*
 * Push N elements after first of a table at index. Returns elements pushed.
 */
static inline int
utils_push_table_data (lua_State *L, int index)
{
    int i, len; 

    luaL_checktype(L, index, LUA_TTABLE);
    len = luaL_len(L, index);

    /* first element in an envelope table is the title */
    for (i = 2; i <= len; i++)
        lua_rawgeti(L, index, i);

    return len - 1;
}

#endif
