#include <stdio.h>
#include "actor.h"
#include "script.h"
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
 * Push the object onto the Actor's state. We pass along the calling state
 * for error handling.
 */
void
script_push_object (Script *script, lua_State *L)
{
    if (!script->is_loaded)
        luaL_error(L, "Script isn't loaded!");
    lua_rawgeti(script->actor->L, LUA_REGISTRYINDEX, script->object_reference);
}

/*
 * Push a table of a Script at index.
 */
void
script_push_table (lua_State *L, Script *script)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, script->table_reference);
}

/*
 * Check the table at index for the requirements of a Script.
 */
void
script_check_table (lua_State *L, int index)
{
    int len;
    luaL_checktype(L, index, LUA_TTABLE);
    len = luaL_len(L, index);
    luaL_argcheck(L, len > 0, index, "Table needs to have a module name!");
}

/*
 * Create a script for an Actor from a table. Returns an unloaded script.
 * Script.new(actor, { "collision", 20, 40 })
 * Script.new(actor, { "weapon", "longsword" })
 */
static int
lua_script_new (lua_State *L)
{
    Script *script = NULL;
    Actor *actor = lua_check_actor(L, 1);
    lua_State *A = actor_request_stack(actor);
    script_check_table(L, 2);

    lua_getglobal(A, "Dialogue");
    lua_getfield(A, -1, "Actor");
    lua_getfield(A, -1, "Script");
    lua_getfield(A, -1, "spawn");
    lua_getglobal(A, "actor");
    utils_copy_top(A, L);
    if (lua_pcall(A, 2, 1, 0))
        luaL_error(L, "Creating new script failed: %s", lua_tostring(A, -1));

    script = lua_check_script(A, -1);
    actor_add_script(actor, script);
    utils_push_object(L, script, SCRIPT_LIB);

    actor_return_stack(actor);
    return 1;
}

/*
 * This is typically an internal method called from Script.new inside an Actor's
 * lua_State. This method is responsible for actually creating a script object.
 */
static int
lua_script_spawn (lua_State *L)
{
    int table_ref;
    Script *script = NULL;
    Actor *actor = lua_check_actor(L, 1);
    script_check_table(L, 2);

    table_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    script = lua_newuserdata(L, sizeof(Script));
    luaL_getmetatable(L, SCRIPT_LIB);
    lua_setmetatable(L, -2);

    script->actor = actor;
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
    int table_index, args = lua_gettop(L);
    const char *module = NULL;
    Script *script = lua_check_script(L, 1);
    lua_State *A = actor_request_stack(script->actor);

    if (script->is_loaded)
        luaL_unref(A, LUA_REGISTRYINDEX, script->object_reference);
    
    /* if they pass in a table, use that as the reference for the module */
    if (args == 2) {
        luaL_unref(A, LUA_REGISTRYINDEX, script->table_reference);
        script_check_table(L, 2);
        utils_copy_top(A, L);
        script->table_reference = luaL_ref(A, LUA_REGISTRYINDEX);
    }

    script_push_table(A, script);
    table_index = lua_gettop(A);

    /* module = require 'module-name' */
    lua_getglobal(A, "require");
    utils_push_table_head(A, table_index);
    module = lua_tostring(A, -1);
    if (lua_pcall(A, 1, 1, 0))
        luaL_error(L, "Require failed for module '%s'", module);

    /* object = module.new(...) */
    lua_getfield(A, -1, "new");
    if (lua_pcall(A, utils_push_table_data(A, table_index), 1, 0)) 
        luaL_error(L, "%s.new() failed", module);

    script->object_reference = luaL_ref(A, LUA_REGISTRYINDEX);
    script->is_loaded = 1;

    actor_return_stack(script->actor);
    return 0;
}

/*
 * Send a script a message from a table.
 * script:send{ "update" }
 */
static int
lua_script_send (lua_State *L)
{
    int argc, envelope_index;
    Script *script = lua_check_script(L, 1);
    lua_State *A;
    luaL_checktype(L, 2, LUA_TTABLE);

    A = actor_request_stack(script->actor);

    utils_copy_table(A, L, 2);
    envelope_index = lua_gettop(A);

    /* function = object.message_title */
    script_push_object(script, L);
    utils_push_table_head(A, envelope_index);
    lua_gettable(A, -2);

    /* it's not an error if the function doesn't exist */
    if (!lua_isfunction(A, -1))
        return 0;

    /* push again to reference 'self' */
    script_push_object(script, L);
    argc = utils_push_table_data(A, envelope_index);
    if (lua_pcall(A, argc + 1, 0, 0)) 
        luaL_error(L, "Error sending message: %s\n", lua_tostring(L, -1));

    actor_return_stack(script->actor);
    return 0;
}

/*
 * A method for querying the Script's object between lua states.
 */
static int
lua_script_probe (lua_State *L)
{
    Script* script = lua_check_script(L, 1);
    const char *element = luaL_checkstring(L, 2);
    lua_State *A = actor_request_stack(script->actor);

    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_reference);
    lua_getfield(A, -1, element);
    utils_copy_top(L, A);
    lua_pop(A, 2);

    actor_return_stack(script->actor);
    return 1;
}

/*
 * Get the table that defines this script.
 */
static int
lua_script_table (lua_State *L)
{
    Script *script = lua_check_script(L, 1);
    lua_State *A = actor_request_stack(script->actor);
    script_push_table(A, script);
    utils_copy_top(L, A);
    actor_return_stack(script->actor);
    return 1;
}

static int
lua_script_tostring (lua_State *L)
{
    Script* script = lua_check_script(L, 1);
    lua_pushfstring(L, "%s %p", SCRIPT_LIB, script);
    return 1;
}

static const luaL_Reg script_methods[] = {
    {"send", lua_script_send},
    {"load", lua_script_load},
    {"probe", lua_script_probe},
    {"table", lua_script_table},
    {"__tostring", lua_script_tostring},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor_Script (lua_State *L)
{
    utils_lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
    lua_pushcfunction(L, lua_script_spawn);
    lua_setfield(L, -2, "spawn");
    return 1;
}
