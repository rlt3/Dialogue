#include "script.h"
#include "actor.h"
#include "utils.h"

/*
 * Check the table at index for the requirements of a Script.
 */
void
lua_check_script_table (lua_State *L, int index)
{
    int len;
    luaL_checktype(L, index, LUA_TTABLE);
    len = luaL_len(L, index);
    luaL_argcheck(L, len > 0, index, "Table needs to have a module name!");
}

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index)
{
    return (Script *) luaL_checkudata(L, index, SCRIPT_LIB);
}

/*
 * Create a Script for an Actor from a table in the given Actor's Lua stack. 
 * Returns an unloaded Script.
 *
 * This is meant to be called from a main Lua state/stack, like an Interpreter
 * and never any Actor's internal stack. We allocate all of our objects in the
 * interpreter state and all the data in the hidden states.
 */
static int
lua_script_new (lua_State *L)
{
    Script *script;
    Actor *actor;
    lua_State *A;

    actor = lua_check_actor(L, 1);
    lua_check_script_table(L, 2);

    script = lua_newuserdata(L, sizeof(*script));
    luaL_getmetatable(L, SCRIPT_LIB);
    lua_setmetatable(L, -2);

    /* Ref the script itself so it doesn't GC */
    script->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* Copy and ref the Script's table definition so we can reload */
    A = actor_request_stack(actor);
    utils_copy_table(A, L, 2);
    script->table_ref = luaL_ref(A, LUA_REGISTRYINDEX);
    script->actor = actor;
    script->next = NULL;
    script->is_loaded = 0;
    actor_return_stack(actor);

    lua_rawgeti(L, LUA_REGISTRYINDEX, script->ref);

    return 1;
}

/*
 * Attempt to load the given Script. *Must* be called from the Actor's thread.
 *
 * Return Codes:
 *   1 - Calling thread isn't the Actor's thread
 *   2 - The module given to the Script isn't valid or has errors
 *   3 - The function 'new' doesn't exist for the module
 *   4 - Calling the function 'new' failed with the arguments given
 */
int
script_load (Script *script)
{
    int table_index;
    int args;
    int ret = 0;
    lua_State *A = script->actor->L;
    pthread_t calling_thread = pthread_self();

    if (calling_thread != script->actor->thread) {
        ret = 1;
        goto exit;
    }

    if (script->is_loaded) {
        luaL_unref(A, LUA_REGISTRYINDEX, script->object_ref);
        script->is_loaded = 0;
    }

    lua_rawgeti(A, LUA_REGISTRYINDEX, script->table_ref);
    table_index = lua_gettop(A);

    /* module = require 'module_name' */
    lua_getglobal(A, "require");
    utils_push_table_head(A, table_index);

    if (lua_pcall(A, 1, 1, 0)) {
        ret = 2;
        goto cleanup;
    }

    /* object = module.new(...) */
    lua_getfield(A, -1, "new");

    if (!lua_isfunction(A, -1)) {
        ret = 3;
        goto cleanup;
    }

    args = utils_push_table_data(A, table_index);

    if (lua_pcall(A, args, 1, 0)) {
        ret = 4;
        goto cleanup;
    }

    script->object_ref = luaL_ref(A, LUA_REGISTRYINDEX);
    script->is_loaded = 1;

cleanup:
    lua_pop(A, 2); /* require & table */
    actor_return_stack(script->actor);

exit:
    return ret;
}

static int
lua_script_load (lua_State *L)
{
    Script *script = lua_check_script(L, 1);

    /*
     * TODO: How do we handle loading just a single Script in the Actor thread
     * and not them all?
     *
     * Maybe it's a simple flag -- look for scripts wanting to be reloaded?
     * Also, do I need mutexes for each Script?
     */

    actor_alert_action(script->actor, LOAD);

    return 0;
}

static int
lua_script_probe (lua_State *L)
{
    lua_State *A;
    Script *script = lua_check_script(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (!script->is_loaded)
        luaL_error(L, "%s %p isn't loaded!", SCRIPT_LIB, script);

    A = actor_request_stack(script->actor);
    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
    lua_getfield(A, -1, field);
    utils_copy_top(L, A);
    lua_pop(A, 2);
    actor_return_stack(script->actor);

    return 1;
}

static const luaL_Reg script_methods[] = {
    {"load",  lua_script_load},
    {"probe", lua_script_probe},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor_Script (lua_State *L)
{
    utils_lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
    return 1;
}
