#include <stdlib.h>
#include "script.h"
#include "actor.h"
#include "utils.h"

/*
 * Check the table at index for the requirements of a Script.
 * Returns 0 if OK.
 * Returns 1 if the table doesn't have a module name.
 * Returns 2 if the table isn't a table!
 */
int
script_table_status (lua_State *A, int index)
{
    if (lua_type(A, index) != LUA_TTABLE)
        return 2;
    return !(luaL_len(A, index) > 0);
}

/*
 * Expects a Script definition on top of the Lua stack A.  Returns a pointer to
 * the Script.
 *
 * If the functions returns NULL, an error string is pushed onto A. Otherwise
 * nothing is pushed onto A and the function returns the Script.
 */
Script *
script_new (lua_State *A)
{
    const int top = -1;
    Script *script = NULL;

    switch (script_table_status(A, top)) {
    case 1:
        /* TODO: serialize table to show mistake that caused error */
        lua_pushfstring(A, "Failed to create script: invalid definition!");
        goto exit;

    case 2:
        lua_pushfstring(A, 
                "Failed to create script: `%s` isn't a table!", 
                lua_tostring(A, top));
        goto exit;

    default:
        break;
    }

    script = malloc(sizeof(*script));

    if (!script) {
        lua_pushstring(A, "Failed to create script: no memory!");
        goto exit;
    }

    /* Copy and ref the Script's table definition so we can reload */
    script->table_ref = luaL_ref(A, LUA_REGISTRYINDEX);

    script->next = NULL;
    script->prev = NULL;

    script->is_loaded = 0;
    script->be_loaded = 1;

exit:
    return script;
}

void
script_destroy (Script *script, lua_State *A)
{
    luaL_unref(A, LUA_REGISTRYINDEX, script->object_ref);
    luaL_unref(A, LUA_REGISTRYINDEX, script->table_ref);
    free(script);
}

/*
 * Assumes the state mutex has been acquired.  Attempt to load the given
 * Script. *Must* be called from the Actor's thread.
 *
 * Returns: LOAD_OK, LOAD_BAD_THREAD, LOAD_FAIL
 *
 * If LOAD_FAIL is returned, the Script is unloaded and turned off.
 * Additionally, details of the error are set if LOAD_FAIL is returned.
 */
int
script_load (Script *script, lua_State *A)
{
//    int table_index;
//    int args;
//    int ret = LOAD_OK;
//    lua_State *A;
//    Actor *actor = script->actor;
//
//    A = actor_request_state(actor);
//
//    /* if an actor has a thread requirement */
//    if (actor->is_lead || actor->is_star) {
//        if (!actor_is_calling_thread(pthread_self())) {
//            script->error = ERR_NOT_CALLING_THREAD;
//            ret = LOAD_BAD_THREAD;
//            goto exit;
//        }
//    }
//
//    if (script->is_loaded) {
//        luaL_unref(A, LUA_REGISTRYINDEX, script->object_ref);
//        script->is_loaded = 0;
//    }
//
//    lua_rawgeti(A, LUA_REGISTRYINDEX, script->table_ref);
//    table_index = lua_gettop(A);
//
//    /* module = require 'module_name' */
//    lua_getglobal(A, "require");
//    utils_push_table_head(A, table_index);
//
//    if (lua_pcall(A, 1, 1, 0)) {
//        script->error = lua_tostring(A, -1);
//        ret = LOAD_FAIL;
//        goto cleanup;
//    }
//
//    /* object = module.new(...) */
//    lua_getfield(A, -1, "new");
//
//    if (!lua_isfunction(A, -1)) {
//        lua_pop(A, 1); /* whatever 'new' is */
//        script->error = ERR_NO_MODULE_NEW;
//        ret = LOAD_FAIL;
//        goto cleanup;
//    }
//
//    args = utils_push_table_data(A, table_index);
//
//    if (lua_pcall(A, args, 1, 0)) {
//        script->error = lua_tostring(A, -1);
//        ret = LOAD_FAIL;
//        goto cleanup;
//    }
//
//    script->object_ref = luaL_ref(A, LUA_REGISTRYINDEX);
//    script->is_loaded = 1;
//
//cleanup:
//    lua_pop(A, 2); /* require & table */
//    script->be_loaded = 0;
//    actor_return_state(script->actor);
//
//    if (ret > 0)
//        printf("Script %p: %s\n", script, script->error);
//
//exit:
//    return ret;
    return 0;
}

/*
 * Assumes the state has been acquired and a message table at -1 in the form of
 *  { 'message' [, arg1 [, ... [, argn]]], author}
 *
 * Returns SEND_OK, SEND_BAD_THREAD, SEND_SKIP, SEND_FAIL.
 *
 * If SEND_FAIL is returned, the Script is unloaded and turned off.
 * Additionally, details of the error are set if SEND_FAIL is returned.
 */
int
script_send (Script *script, lua_State *A)
{
//    int message_index;
//    int args;
//    int ret = SEND_OK;
//    lua_State *A = script->actor->L;
//
//    /* if an actor has a thread requirement */
//    if (script->actor->is_lead || script->actor->is_star) {
//        if (!actor_is_calling_thread(pthread_self())) {
//            ret = SEND_BAD_THREAD;
//            goto exit;
//        }
//    }
//
//    message_index = lua_gettop(A);
//
//    /* function = object.message_title */
//    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
//    utils_push_table_head(A, message_index);
//    lua_gettable(A, -2);
//
//    /* it's not an error if the function doesn't exist */
//    if (!lua_isfunction(A, -1)) {
//        ret = SEND_SKIP;
//        lua_pop(A, 2);
//        goto exit;
//    }
//
//    /* push `self' reference and tail of message which includes the author. */
//    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
//    args = utils_push_table_data(A, message_index);
//
//    if (lua_pcall(A, args + 1, 0, 0)) {
//        ret = SEND_FAIL;
//        script->error = lua_tostring(A, -1);
//        printf("%s\n", script->error);
//        script_unload(script);
//    }
//    
//    lua_pop(A, 1);
//
//exit:
//    return ret;
    return 0;
}

/*
 * Sets the Script to be reloaded. If an optional table is passed it, this
 * method uses that table as the definition table from which it is loaded.
 */
static int
lua_script_load (lua_State *L)
{
//    //lua_State *A;
//    //int new_definition = 0;
//    //int args = lua_gettop(L);
//    Script *script = lua_check_script(L, 1);
//
//    script_load(script);
//
//    //if (args == 2) {
//    //    lua_check_script_table(L, 2);
//    //    new_definition = 1;
//    //}
//
//    //A = actor_request_state(script->actor);
//    //if (new_definition) {
//    //    luaL_unref(A, LUA_REGISTRYINDEX, script->table_ref);
//    //    utils_copy_top(A, L);
//    //    script->table_ref = luaL_ref(A, LUA_REGISTRYINDEX);
//    //}
//    //script->be_loaded = 1;
//    //actor_return_state(script->actor);
//
//    return luaf(L, "Dialogue.Post.send(%1:actor, 'load')");
    return 0;
}

/*
 * Access a field and get the results from the object inside the Script.
 *
 * script:probe("coordinates") => {100, 50}
 * script:probe("weapon") => "axe"
 * script:probe("following") => Dialogue.Actor 0x003f9bc39
 */
static int
lua_script_probe (lua_State *L)
{
//    lua_State *A;
//    Script *script = lua_check_script(L, 1);
//    const char *field = luaL_checkstring(L, 2);
//
//    A = actor_request_state(script->actor);
//
//    if (!script->is_loaded) {
//        actor_return_state(script->actor);
//        lua_script_print_error(L, script, "Cannot Probe");
//    }
//
//    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
//    lua_getfield(A, -1, field);
//    utils_copy_top(L, A);
//    lua_pop(A, 2);
//
//    actor_return_state(script->actor);
//
    return 1;
}

/*
 * Remove a script from the Actor's state.
 */
static int
lua_script_remove (lua_State *L)
{
//    lua_State *A;
//    Script *script = lua_check_script(L, 1);
//
//    A = actor_request_state(script->actor);
//    actor_remove_script(script->actor, script);
//    script_unload(script);
//    actor_return_state(script->actor);
//
    return 0;
}
