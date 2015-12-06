#include "tmp/actor.h"
#include "tmp/actor_thread.h"
#include "tmp/script.h"

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
 */
static int
lua_script_new (lua_State *L)
{
    Script *script;
    Actor *actor;
    lua_State *A;

    actor = lua_check_actor(L, 1);
    lua_check_script_table(L, 2);

    A = actor_request_stack(actor);

    script = lua_newuserdata(A, sizeof(Script));
    luaL_getmetatable(A, SCRIPT_LIB);
    lua_setmetatable(A, -2);

    /* Copy and ref the Script's table definition so we can reload */
    utils_copy_top(A, L);
    script->table_ref = luaL_ref(A, LUA_REGISTRYINDEX);

    /* Ref the script itself so it doesn't GC */
    script->ref = luaL_ref(A, LUA_REGISTRYINDEX);

    script->actor = actor;
    script->next = NULL;
    script->is_loaded = 0;

    /* if we're calling from the same state, just push the ref */
    if (L == A) {
        lua_rawgeti(A, LUA_REGISTRYINDEX, script->ref);
    } else {
        utils_object_push(L, script, SCRIPT_LIB);
    }

    actor_return_stack(actor);

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
    module_name = lua_tostring(A, -1);

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

    script->object_reference = luaL_ref(A, LUA_REGISTRYINDEX);
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
