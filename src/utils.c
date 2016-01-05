#include "utils.h"

/*
 * Push a pointer and associate a metatable with it.
 */
void
utils_push_object (lua_State *L, void *object_ptr, const char *metatable)
{
    lua_pushlightuserdata(L, object_ptr);
    luaL_getmetatable(L, metatable);
    lua_setmetatable(L, -2);
}

/*
 * Push an object's method and also the object to reference `self`.
 */
void
utils_push_object_method (lua_State *L, 
        void *object_ptr, 
        const char *metatable, 
        const char *method)
{
    utils_push_object(L, object_ptr, metatable);
    lua_getfield(L, -1, method);
    utils_push_object(L, object_ptr, metatable);
}

/*
 * Push an object reference and prep a method call.
 */
void
utils_push_objref_method (lua_State *L, int ref, const char *method)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_getfield(L, -1, method);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}

/*
 * Add a slot to a userdata object and modifying its __index.
 */
void
utils_add_method (lua_State *L, int index, lua_CFunction f, const char* field)
{
    lua_getfield(L, index, "__index");
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, field);
    lua_pop(L, 1);
}

/*
 * Push a sub table from indices start..end of table at index.
 */
void
utils_push_table_sub (lua_State *L, int table_index, int start)
{
    int i, len = luaL_len(L, table_index);
    int new_index = 1;
    lua_newtable(L);
    for (i = start; i <= len; i++) {
        lua_rawgeti(L, table_index, i);
        lua_rawseti(L, -2, new_index++);
    }
}

/*
 * Remove the first element in a table at given index and leave it on stack.
 */
void
utils_pop_table_head (lua_State *L, int index)
{
    /* remove like queue LIFO */
    lua_getglobal(L, "table");
    lua_getfield(L, -1, "remove");
    lua_pushvalue(L, index);
    lua_pushinteger(L, 1);
    lua_call(L, 2, 1);

    /* move the 'table' on top to pop */
    lua_insert(L, lua_gettop(L) - 1);
    lua_pop(L, 1);
}

/*
 * Push the first element of a table at index.
 */
void
utils_push_table_head (lua_State *L, int index)
{
    lua_rawgeti(L, index, 1);
}

/*
 * Push N elements after first of a table at index. Returns elements pushed.
 */
int
utils_push_table_data (lua_State *L, int index)
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
utils_copy_table (lua_State *to, lua_State *from, int index)
{
    int i;
    lua_newtable(to);
    lua_pushnil(from);
    for (i = 1; lua_next(from, index); i++) {
        utils_copy_top(to, from);
        lua_rawseti(to, -2, i);
        lua_pop(from, 1);
    }
}

/*
 * Copies the value at the top of 'from' to 'to'.
 *
 * Copies Number, String, Boolean, and Table types specifically. All other 
 * types including Userdata are probed for the __tostring metamethod. If it
 * exists, it uses the string output to copy to the next stack. Else if just
 * pushes nil.
 */
void
utils_copy_top (lua_State *to, lua_State *from)
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
        lua_getfield(from, -1, "__tostring");                                      
        lua_pushvalue(from, -2);                                                   
        lua_call(from, 1, 1);                                                      
        utils_copy_top(to, from);                                                  
        lua_pop(from, 1);                                                          
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
utils_lua_meta_open (lua_State *L, 
        const char *metatable, 
        const luaL_Reg *methods, 
        lua_CFunction new)
{
    /* create metatable */
    luaL_newmetatable(L, metatable);

    /* metatable.__index = metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    /* register methods */
    luaL_setfuncs(L, methods, 0);

    lua_newtable(L);
    lua_pushcfunction(L, new);
    lua_setfield(L, -2, "new");

    return 1;
}
