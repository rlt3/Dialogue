#include <stdio.h>
#include "script.h"
#include "actor.h"
#include "envelope.h"
#include "utils.h"

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index)
{
    return (Script *) luaL_checkudata(L, index, SCRIPT_LIB);
}

/*
 * Push the object of a Script at index. If object isn't loaded, throws error.
 */
void
script_push_object (lua_State *L, int index)
{
    Script *script = lua_check_script(L, index);
    if (!script->is_loaded)
        luaL_error(L, "Script isn't loaded!");
    lua_rawgeti(L, LUA_REGISTRYINDEX, script->object_reference);
}

/*
 * Push a table of a Script at index.
 */
void
script_push_table (lua_State *L, int index)
{
    Script *script = lua_check_script(L, index);
    lua_rawgeti(L, LUA_REGISTRYINDEX, script->table_reference);
}

/*
 * Create a script from a table.
 * Script{ "collision", 20, 40 }
 * Script{ "weapon", "longsword" }
 */
static int
lua_script_new (lua_State *L)
{
    int len, table_ref;
    Script *script = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);
    len = luaL_len(L, 1);
    luaL_argcheck(L, len > 0, 1, "Script needs to have a module name!");

    table_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    script = lua_newuserdata(L, sizeof(Script));
    luaL_getmetatable(L, SCRIPT_LIB);
    lua_setmetatable(L, -2);

    script->table_reference = table_ref;
    script->next = NULL;
    script->is_loaded = 0;

    return 1;
}

/*
 * Load (or reload) a script's module with the data given.
 */
static int
lua_script_load (lua_State *L)
{
    int args;
    const char *module = NULL;
    Script *script = NULL;

    script = lua_check_script(L, 1);
    script_push_table(L, 1);         /* 2 */

    /* module = require 'module-name' */
    lua_getglobal(L, "require");
    table_push_head(L, 2);
    module = lua_tostring(L, -1);
    if (lua_pcall(L, 1, 1, 0))       /* 3 */
        luaL_error(L, "Require failed for module %s", module);

    /* object = module.new(...) */
    lua_getfield(L, 3, "new");
    args = table_push_data(L, 2);
    if (lua_pcall(L, args, 1, 0)) 
        luaL_error(L, "%s.new() failed: %s", module, lua_tostring(L, -1));

    script->object_reference = luaL_ref(L, LUA_REGISTRYINDEX);
    script->is_loaded = 1;

    return 0;
}


/*
 * Send a script a message from a table.
 * script:send{ "update" }
 */
static int
lua_script_send (lua_State *L)
{
    int argc;

    /* object:message_title(...) */
    script_push_object(L, 1);  /* 3 */
    luaL_checktype(L, 2, LUA_TTABLE);
    table_push_head(L, 2);
    lua_gettable(L, 3);

    /* it's not an error if the function doesn't exist */
    if (!lua_isfunction(L, -1))
        return 0;

    /* push again to reference 'self' */
    script_push_object(L, 1);
    argc = table_push_data(L, 2);
    if (lua_pcall(L, argc + 1, 0, 0)) 
        luaL_error(L, "Error sending message: %s\n", lua_tostring(L, -1));

    return 0;
}

/*
 * A method for querying the Script's object between lua states. This isn't
 * threadsafe and should only be used for debugging.
 *
 * script:probe("x") => object["x"] => 6
 */
static int
lua_script_probe (lua_State *L)
{
    int type;
    Script* script = lua_check_script(L, 1);
    const char *element = luaL_checkstring(L, 2);
    lua_State *A = script->actor->L;

    lua_pop(A, lua_gettop(A));
    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_reference);
    lua_getfield(A, -1, element);

    type = lua_type(A, -1);
    switch(type)
    {
    case LUA_TNUMBER: /* assume integer always */
        lua_pushnumber(L, lua_tonumber(A, -1));
        break;
        
    case LUA_TSTRING:
        lua_pushstring(L, lua_tostring(A, -1));
        break;

    case LUA_TBOOLEAN:
        lua_pushinteger(L, lua_tointeger(A, -1));
        break;

    case LUA_TTABLE:
        lua_pushstring(L, "Table");
        break;

    default:
        lua_pushnil(L);
        break;
    }
        
    lua_pop(A, lua_gettop(A));

    return 1;
}

static int
lua_script_tostring (lua_State *L)
{
    Script* script = lua_check_script(L, 1);
    lua_pushfstring(L, "%s %p", SCRIPT_LIB, script);
    return 1;
}

static int
lua_script_gc (lua_State *L)
{
    Script *script = lua_check_script(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, script->table_reference);
    luaL_unref(L, LUA_REGISTRYINDEX, script->object_reference);
    return 0;
}

static const luaL_Reg script_methods[] = {
    {"send",  lua_script_send},
    {"load",  lua_script_load},
    {"probe", lua_script_probe},
    {"__gc",  lua_script_gc},
    {"__tostring", lua_script_tostring},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor_Script (lua_State *L)
{
    return lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
}
