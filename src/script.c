#include <stdio.h>
#include "script.h"
#include "utils.h"

struct Script {
    int table_reference;
};

Script *
lua_check_script (lua_State *L, int index)
{
    return (Script *) luaL_checkudata(L, index, SCRIPT_LIB);
}

/*
 * Create a script from a table.
 * Script{ "collision", 20, 40 }
 * Script{ "weapon", "longsword" }
 */
static int
lua_script_new (lua_State *L)
{
    int reference;
    const char *module_name = NULL;
    Script *script = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);

    /* Push new table and then swap positions with table on top. */
    reference = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_newtable(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    /* t["construct"] = { "draw", "player.jpg", 200, 400 } */
    lua_setfield(L, -2, "construct");
    luaL_unref(L, LUA_REGISTRYINDEX, reference);

    /* module_name = t["construct"][1] */
    lua_getfield(L, -1, "construct");
    lua_rawgeti(L, -1, 1);
    module_name = lua_tostring(L, -1);
    lua_pop(L, 2);

    /* t.module = require(module_name) */
    lua_getglobal(L, "require");
    lua_pushstring(L, module_name);
    if (lua_pcall(L, 1, 2, 0)) /* returns 2 things, status and module table */
        printf("Error loading script: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1); /* pop the status with no checks to module on top */
    lua_setfield(L, -2, "module");

    /* t.object = t.module.new() */
    lua_getfield(L, -1, "module");
    lua_getfield(L, -1, "new");
    if (lua_pcall(L, 0, 1, 0)) 
        printf("Error loading script: %s\n", lua_tostring(L, -1));
    lua_setfield(L, -3, "object");
    lua_pop(L, 1);

    /* reference & pop our created table so we can push our object */
    reference = luaL_ref(L, LUA_REGISTRYINDEX);

    script = lua_newuserdata(L, sizeof(Script));
    script->table_reference = reference;
    luaL_getmetatable(L, SCRIPT_LIB);
    lua_setmetatable(L, -2);
    return 1;
}

static int
lua_script_send (lua_State *L)
{
    Script *script = lua_check_script(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, script->table_reference);
    
    /* object:message() */
    lua_getfield(L, -1, "object");
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    lua_getfield(L, -3, "object"); /* object methods get `self` from this */
    
    if (lua_pcall(L, 1, 0, 0)) 
        printf("Error loading script: %s\n", lua_tostring(L, -1));

    return 0;
}

static int
lua_script_table (lua_State *L)
{
    Script *script = lua_check_script(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, script->table_reference);
    return 1;
}

static int
lua_script_gc (lua_State *L)
{
    Script *script = lua_check_script(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, script->table_reference);
    return 0;
}

static const luaL_Reg script_methods[] = {
    {"table", lua_script_table},
    {"send",  lua_script_send},
    {"__gc",  lua_script_gc},
    { NULL, NULL }
};

int 
luaopen_Script (lua_State *L)
{
    return lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
}
