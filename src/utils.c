#include "utils.h"

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
