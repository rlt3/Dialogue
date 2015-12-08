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
 * Print the error of a Script with a given prefix. Assumes access to state 
 * mutex.
 */
void
lua_script_print_error (lua_State *L, Script *s, const char *prefix)
{
    luaL_error(L, "%s: %s", prefix, s->error);
}

/*
 * Assumes access to state & stack mutexes. Unloads the Script making it exist
 * as dead weight. It will be skipped by messages and load attempts unless 
 * changed manually.
 */
void
script_unload (Script *script)
{
    lua_State *A = script->actor->L;
    luaL_unref(A, LUA_REGISTRYINDEX, script->object_ref);
    script->is_loaded = 0;
    script->be_loaded = 0;
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

    /* Copy and ref the Script's table definition so we can reload */
    A = actor_request_state(actor);
    utils_copy_table(A, L, 2);
    script->table_ref = luaL_ref(A, LUA_REGISTRYINDEX);
    script->actor = actor;
    script->next = NULL;
    script->prev = NULL;
    script->is_loaded = 0;
    script->be_loaded = 1;
    script->error = NULL;
    actor_return_state(actor);

    return 1;
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
script_load (Script *script)
{
    int table_index;
    int args;
    int ret = LOAD_OK;
    lua_State *A;
    pthread_t calling_thread = pthread_self();

    A = actor_request_state(script->actor);

    if (calling_thread != script->actor->thread) {
        script->error = ERR_NOT_CALLING_THREAD;
        ret = LOAD_BAD_THREAD;
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
        script->error = ERR_BAD_MODULE;
        ret = LOAD_FAIL;
        goto cleanup;
    }

    /* object = module.new(...) */
    lua_getfield(A, -1, "new");

    if (!lua_isfunction(A, -1)) {
        lua_pop(A, 1); /* whatever 'new' is */
        script->error = ERR_NO_MODULE_NEW;
        ret = LOAD_FAIL;
        goto cleanup;
    }

    args = utils_push_table_data(A, table_index);

    if (lua_pcall(A, args, 1, 0)) {
        lua_pop(A, 1); /* error */
        printf("ERR_BAD_MODULE_NEW top: %d\n", lua_gettop(A));
        script->error = ERR_BAD_MODULE_NEW;
        ret = LOAD_FAIL;
        goto cleanup;
    }

    script->object_ref = luaL_ref(A, LUA_REGISTRYINDEX);
    script->is_loaded = 1;

cleanup:
    lua_pop(A, 2); /* require & table */
    script->be_loaded = 0;
    actor_return_state(script->actor);

exit:
    return ret;
}

/*
 * Assumes the state has been acquired and there is a message at the top of the
 * stack.
 *
 * Returns SEND_OK, SEND_BAD_THREAD, SEND_SKIP, SEND_FAIL.
 *
 * If SEND_FAIL is returned, the Script is unloaded and turned off.
 * Additionally, details of the error are set if SEND_FAIL is returned.
 */
int
script_send (Script *script)
{
    int message_index;
    int args;
    int ret = SEND_OK;
    lua_State *A = script->actor->L;
    pthread_t calling_thread = pthread_self();

    if (calling_thread != script->actor->thread) {
        ret = SEND_BAD_THREAD;
        goto exit;
    }

    message_index = lua_gettop(A);

    /* function = object.message_title */
    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
    utils_push_table_head(A, message_index);
    lua_gettable(A, -2);

    /* it's not an error if the function doesn't exist */
    if (!lua_isfunction(A, -1)) {
        ret = SEND_SKIP;
        lua_pop(A, 2);
        goto exit;
    }

    /* push `self' reference, args, and the author reference */
    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
    args = utils_push_table_data(A, message_index);
    //utils_push_object(A, author, ACTOR_LIB);

    if (lua_pcall(A, args + 1, 0, 0)) {
        ret = SEND_FAIL;
        script->error = lua_tostring(A, -1);
        script_unload(script);
    }
    
    lua_pop(A, 1);

exit:
    return ret;
}

/*
 * Manually reload the Script. An optional table can be passed in if a new
 * table definition is required.
 */
static int
lua_script_load (lua_State *L)
{
    lua_State *A;
    int new_definition = 0;
    int args = lua_gettop(L);
    Script *script = lua_check_script(L, 1);

    if (args == 2) {
        lua_check_script_table(L, 2);
        new_definition = 1;
    }

    A = actor_request_state(script->actor);

    if (new_definition) {
        luaL_unref(A, LUA_REGISTRYINDEX, script->table_ref);
        utils_copy_top(A, L);
        script->table_ref = luaL_ref(A, LUA_REGISTRYINDEX);
    }

    script->be_loaded = 1;

    actor_return_state(script->actor);
    actor_alert_action(script->actor, LOAD);

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
    lua_State *A;
    Script *script = lua_check_script(L, 1);
    const char *field = luaL_checkstring(L, 2);

    A = actor_request_state(script->actor);

    if (!script->is_loaded) {
        actor_return_state(script->actor);
        lua_script_print_error(L, script, "Cannot Probe");
    }

    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
    lua_getfield(A, -1, field);
    utils_copy_top(L, A);
    lua_pop(A, 2);

    actor_return_state(script->actor);

    return 1;
}

/*
 * Remove a script from the Actor's state.
 */
static int
lua_script_remove (lua_State *L)
{
    lua_State *A;
    Script *script = lua_check_script(L, 1);

    A = actor_request_state(script->actor);
    actor_remove_script(script->actor, script);
    script_unload(script);
    actor_return_state(script->actor);

    return 0;
}

static int
lua_script_error (lua_State *L)
{
    Script* script = lua_check_script(L, 1);

    actor_request_state(script->actor);
    if (script->error == NULL) {
        lua_pushstring(L, "No error");
    } else {
        lua_pushstring(L, script->error);
    }
    actor_return_state(script->actor);
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
    {"load",       lua_script_load},
    {"probe",      lua_script_probe},
    {"remove",     lua_script_remove},
    {"error",      lua_script_error},
    {"__tostring", lua_script_tostring},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor_Script (lua_State *L)
{
    utils_lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
    return 1;
}
