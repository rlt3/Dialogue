#include <stdio.h>
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
 * Expects a Script table at index. Pushes the module onto the stack.
 */
void
script_push_module (lua_State *L, int index)
{
    lua_rawgeti(L, index, 1);
}

/*
 * Expects a Script table at index. Pushes all data onto the stack. Returns
 * the number of args pushed.
 */
int
script_push_data (lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int i, len = luaL_len(L, index);

    /* first element in an script table is the module */
    for (i = 2; i <= len; i++)
        lua_rawgeti(L, index, i);

    return len - 1;
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

    lua_getglobal(L, "require");
    script_push_module(L, 2);
    module = lua_tostring(L, -1);
    if (lua_pcall(L, 1, 1, 0))       /* 3 */
        luaL_error(L, "Require failed for module %s", module);

    lua_getfield(L, 3, "new");
    args = script_push_data(L, 2);
    if (lua_pcall(L, args, 1, 0)) 
        luaL_error(L, "%s.new() failed: %s", module, lua_tostring(L, -1));

    script->object_reference = luaL_ref(L, LUA_REGISTRYINDEX);
    script->is_loaded = 1;

    return 0;
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
        luaL_error(L, "Error sending message: %s\n", lua_tostring(L, -1));

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
    {"load",  lua_script_load},
    {"__gc",  lua_script_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor_Script (lua_State *L)
{
    return lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
}
