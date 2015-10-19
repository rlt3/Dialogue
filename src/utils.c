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
 * Push an object's method and also the object to reference `self`.
 */
void
lua_method_push (lua_State *L, 
        void *object_ptr, 
        const char *metatable, 
        const char *method)
{
    lua_object_push(L, object_ptr, metatable);
    lua_getfield(L, -1, method);
    lua_object_push(L, object_ptr, metatable);
}

/*
 * Push the first element of a table at index.
 */
void
table_push_head (lua_State *L, int index)
{
    lua_rawgeti(L, index, 1);
}

/*
 * Push N elements after first of a table at index. Returns elements pushed.
 */
int
table_push_data (lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int i, len = luaL_len(L, index);

    /* first element in an envelope table is the title */
    for (i = 2; i <= len; i++)
        lua_rawgeti(L, index, i);

    return len - 1;
}

/*
 * Pushes table onto 'to' stack that's a copy of table in 'from' at index.
 */
void
table_push_copy (lua_State *from, lua_State *to, int index)
{
    int i;
    lua_newtable(to);
    lua_pushnil(from);
    for (i = 1; lua_next(from, index); i++) {
        lua_copy_top(from, to);
        lua_rawseti(to, -2, i);
        lua_pop(from, 1);
    }
}

/*
 * Copies the value at the top of 'from' to 'to'.
 */
void
lua_copy_top (lua_State *from, lua_State *to)
{
    int type = lua_type(from, -1);
    switch(type)
    {
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
        table_push_copy(from, to, lua_gettop(from));
        break;

    default:
        lua_pushnil(to);
        break;
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
