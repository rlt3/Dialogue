#include <stdlib.h>
#include "script.h"
#include "actor.h"
#include "utils.h"
#include <assert.h>

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
script_destroy (Script *script)
{
    script->prev = NULL;
    script->next = NULL;
    free(script);
}

/*
 * Loads (or reloads) the Script created in the given Lua stack.  
 *
 * Assume a Script definition table (table_ref) has the form of
 *  { 'module' [, arg1 [, ... [, argn]]] }
 *
 * The module is `required' and then the function `new` is called on the table
 * that is returned from `require`. The args supplied in the script definition
 * are passed into the `new` function.
 *
 * Returns 0 if successful, 1 if an error occurs. If an error occurs, an error
 * string is pushed onto A.
 */
int
script_load (Script *script, lua_State *A)
{
    const char *module_name;
    const int table_index = lua_gettop(A) + 1; /* we push it onto the stack */
    int args, ret = 1;

    if (script->is_loaded)
        script_unload(script, A);

    lua_rawgeti(A, LUA_REGISTRYINDEX, script->table_ref);

    /* module = require 'module_name' */
    lua_getglobal(A, "require");
    utils_push_table_head(A, table_index);
    module_name = lua_tostring(A, -1);

    if (lua_pcall(A, 1, 1, 0)) {
        lua_pushfstring(A, "Cannot load module `%s': %s", 
                module_name, lua_tostring(A, -1));
        /* push error message beneath pcall error and table */
        lua_insert(A, lua_gettop(A) - 2);
        lua_pop(A, 2); /* pcall error and table */
        goto exit;
    }

    /* object = module.new(...) */
    lua_getfield(A, -1, "new");

    if (!lua_isfunction(A, -1)) {
        lua_pop(A, 3); /* 'new', item returned by `require`, and table */
        lua_pushfstring(A, "Cannot load module `%s': `new' is not a function!", 
                module_name);
        goto exit;
    }

    args = utils_push_table_data(A, table_index);

    if (lua_pcall(A, args, 1, 0)) {
        lua_pop(A, 2); /* pcall error, item returned by `require`, and table */
        lua_pushfstring(A, 
                "Cannot load module `%s': call to `new` failed with args", 
                module_name);
        goto exit;
    }

    script->object_ref = luaL_ref(A, LUA_REGISTRYINDEX);
    script->is_loaded = 1;
    ret = 0;
    lua_pop(A, 2); /* table returned from require and definition table */

exit:
    script->be_loaded = 0;
    return ret;
}

/*
 * Sends a Message to the object created from script_load.
 *
 * Assumes a message definition table at -1 in the form of
 *  { 'message' [, arg1 [, ... [, argn]]], author}
 *
 * The 'message' is some method of the instantiated object which was created in
 * script_load. If it doesn't exist it actually isn't an error. This is one of
 * those "it's a feature, not a bug" things -- by willing to say this isn't an
 * error we gain the ability to very easily add new message primitives (the
 * methods themselves) to the system.
 *
 * Returns 0 if successful, 1 if an error occurs. If an error occurs, an error
 * string is pushed onto A.
 */
int
script_send (Script *script, lua_State *A)
{
    const int message_index = lua_gettop(A);
    const int object_index = message_index + 1;
    const char *message = NULL;
    int args = 0;
    int ret = 1;

    /* 
     * object[message_title]:(arg1, arg2, ..., argN)
     */
    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);

    utils_push_table_head(A, message_index);
    message = lua_tostring(A, -1);
    lua_gettable(A, -2);

    /* it's not an error if the function doesn't exist */
    if (!lua_isfunction(A, -1)) {
        lua_pop(A, 2); /* whatever isn't a function and the object_ref */
        goto success;
    }

    /* push `self' reference and tail of message which includes the author. */
    lua_pushvalue(A, object_index);
    args = utils_push_table_data(A, message_index);

    if (lua_pcall(A, args + 1, 0, 0)) {
        /* TODO: Figure out why the 'Cannot send message' isn't appearing */
        lua_pushfstring(A, "Cannot send message `%s': %s", 
                message, lua_tostring(A, -1));
        /* push error message beneath pcall error and object_ref */
        lua_insert(A, lua_gettop(A) - 2);
        lua_pop(A, 2); /* pcall error and object_ref */
        
        /* unload after the object has been popped or it won't gc */
        script_unload(script, A);
        goto exit;
    }
    
    lua_pop(A, 1); /* object_ref */

success:
    ret = 0;
exit:
    return ret;
}

/*
 * Access a field and get the results from the object inside the Script.
 *
 * If there is an error this function returns 1 and leaves an error string on
 * top of the Actor's stack. Returns 0 if successful and leaves the probed
 * value on top of the Actor's stack.
 */
int
script_probe (Script *script, lua_State *A, const char *field)
{
    int ret = 1;

    if (!script->is_loaded) {
        lua_pushfstring(A, "Cannot probe `%s': not loaded!", field);
        goto exit;
    }

    lua_rawgeti(A, LUA_REGISTRYINDEX, script->object_ref);
    lua_getfield(A, -1, field);
    lua_insert(A, 1);
    lua_pop(A, 1);

    ret = 0;
exit:
    return ret;
}

/*
 * Unload a loaded script. This manually calls garbage collection on A. If the 
 * actor has a thread requirement, this function must be called in the correct
 * thread.
 */
void
script_unload (Script *script, lua_State *A)
{
    if (!script->is_loaded)
        return;

    luaL_unref(A, LUA_REGISTRYINDEX, script->object_ref);
    lua_gc(A, LUA_GCCOLLECT, 0);

    script->is_loaded = 0;
    script->be_loaded = 0;
}


/*
 * Remove a script from the Actor's state.
 */
int
lua_script_remove (lua_State *L)
{
    return 0;
}

int
lua_script_new (lua_State *L)
{
    const int args = lua_gettop(L);
    const char *meta_string = lua_tostring(L, lua_upvalueindex(1));

    lua_pushvalue(L, lua_upvalueindex(2));

    if (lua_isnil(L, -1) || !lua_isfunction(L, -1)) {
        lua_newtable(L);
        goto set_meta;
    }

    lua_insert(L, 1);
    lua_call(L, args, 1);
    luaL_checktype(L, -1, LUA_TTABLE);

set_meta:
    luaL_getmetatable(L, meta_string);
    lua_setmetatable(L, -2);

    return 1;
}

/*
 * Script(script_name, script_init)
 *
 * Script looks at the first argument to use as a metatable name and creates a
 * new metatable. It attaches the function `new` to that metatable.
 *
 * The `new` function accepts any number of arguments and passes them to the 
 * anonymous function which is the second argument of `Script`. `new` then
 * accepts the table output of the anonymous function, attaches the correct
 * metatable to it and returns it.
 */
int
lua_script (lua_State *L)
{
    const char *script_name = NULL;
    const int script_arg = 1;
    const int func_arg = 2;

    luaL_checktype(L, script_arg, LUA_TSTRING);
    luaL_checktype(L, func_arg, LUA_TFUNCTION);

    script_name = lua_tostring(L, script_arg);

    luaL_newmetatable(L, script_name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    lua_pushvalue(L, script_arg);
    lua_pushvalue(L, func_arg);
    lua_pushcclosure(L, lua_script_new, 2);
    lua_setfield(L, -2, "new");

    return 1;
}

/*
 * Set the `Script` Lua function in the Actor's state, which is the constructor
 * for all of the Scripts in Dialogue.
 */
void
script_set (lua_State *A)
{
    lua_pushcfunction(A, lua_script);
    lua_setglobal(A, "Script");
}
