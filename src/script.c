#include <stdio.h>
#include "envelope.h"
#include "script.h"
#include "utils.h"

struct Script {
    int table_reference;
    int object_reference;
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
    int len;
    int table_ref, object_ref;
    const char *module_name = NULL;
    Script *script = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);
    len = luaL_len(L, 1);
    luaL_argcheck(L, len > 0, 1, "Script needs to have a module name!");

    /* Push new table and then swap positions with table on top. */
    table_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_newtable(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, table_ref);

    /* t["construct"] = { "draw", "player.jpg", 200, 400 } */
    lua_setfield(L, -2, "construct");
    luaL_unref(L, LUA_REGISTRYINDEX, table_ref);

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
    object_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);

    /* reference & pop our created table so we can push our object */
    table_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    script = lua_newuserdata(L, sizeof(Script));
    script->table_reference = table_ref;
    script->object_reference = object_ref;
    luaL_getmetatable(L, SCRIPT_LIB);
    lua_setmetatable(L, -2);
    return 1;
}

/*
 * Checks for a Script at index.
 * Push the script's object onto the stack.
 */
void
script_push_object (lua_State *L, int index)
{
    Script *script = lua_check_script(L, index);
    lua_rawgeti(L, LUA_REGISTRYINDEX, script->object_reference);
}

static int
lua_script_send (lua_State *L)
{
    int argc;

    envelope_push_table(L, 2); /* 3 */
    script_push_object(L, 1);  /* 4 */
    
    /* function = object:message_title */
    envelope_push_title(L, 3);
    lua_gettable(L, 4);

    /* push again to reference 'self' */
    script_push_object(L, 1);

    /* function (message_data) */
    argc = envelope_push_data(L, 3);
    
    if (lua_pcall(L, argc + 1, 0, 0)) 
        printf("Error sending message: %s\n", lua_tostring(L, -1));

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
luaopen_Dialogue_Script (lua_State *L)
{
    return lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
}
