#include <stdio.h>
#include "envelope.h"
#include "script.h"
#include "utils.h"

struct Script {
    int table_reference;
    int object_reference;
};

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index)
{
    return (Script *) luaL_checkudata(L, index, SCRIPT_LIB);
}

/*
 * Push the object of a Script at index.
 */
void
script_push_object (lua_State *L, int index)
{
    Script *script = lua_check_script(L, index);
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
 * Expects a Script table at index. Return the module name from the table.
 * Doesn't push to the stack.
 */
const char *
script_module_string (lua_State *L, int index)
{
    const char *module = NULL;
    lua_rawgeti(L, index, 1);
    module = lua_tostring(L, -1);
    lua_pop(L, 1);
    return module;
}

/*
 * Get the module's name and load it. Doesn't push to the stack.
 */
void
script_module_load (lua_State *L, int index)
{
    int ref, error;
    Script *script = NULL;
    const char* module =  NULL;

    script = lua_check_script(L, index);
    script_push_table(L, index);
    module = script_module_string(L, -1);

    /* status, module = require(module_name) */
    lua_getglobal(L, "require");
    lua_pushstring(L, module);
    if (lua_pcall(L, 1, 2, 0))
        luaL_error(L, "Require failed for module: %s", module);

    error = lua_toboolean(L, -1);
    if (error)
        luaL_error(L, "%s module failed to load: %s", module);
    lua_pop(L, 1); /* pop status we just checked */

    /* module.new() */
    lua_getfield(L, -1, "new");
    if (lua_pcall(L, 0, 1, 0)) 
        luaL_error(L, "`new' failed for module: %s", module);

    ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 2); /* pop module and script table */

    script->object_reference = ref;
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
    script_module_load(L, -1);

    return 1;
}

/*
 * Send a script a message from an envelope.
 */
static int
lua_script_send (lua_State *L)
{
    int argc;

    envelope_push_table(L, 2); /* 3 */
    script_push_object(L, 1);  /* 4 */
    
    /* function = object:message_title */
    envelope_push_title(L, 3);
    lua_gettable(L, 4);

    /* it's not an error if the function doesn't exist */
    if (!lua_isfunction(L, -1))
        return 0;

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
    script_push_table(L, 1);
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
