#include "script.h"
#include "utils.h"

struct Script {
    int table_reference;
    Script *prev;
    Script *next;
};

Script *
lua_check_script (lua_State *L, int index)
{
    return (Script *) luaL_checkudata(L, index, SCRIPT_LIB);
}

/*
 * Create a new Script from a module and return it.
 * Script("collision") => script{module = "collision"}
 */
static int
lua_script_new (lua_State *L)
{
    //int i, argc = lua_gettop(L);
    const char *module_name = luaL_checkstring(L, 1);
    Script *script = lua_newuserdata(L, sizeof(Script));
    script->prev = NULL;
    script->next = NULL;

    lua_newtable(L);

    /* t["module"] = module_name */
    lua_pushstring(L, module_name);
    lua_setfield(L, -2, "module");

    /* this pops the new table we made */
    script->table_reference = luaL_ref(L, LUA_REGISTRYINDEX);

    /* push our Script userdata on top of the stack as an object */
    luaL_getmetatable(L, SCRIPT_LIB);
    lua_setmetatable(L, -2);
    return 1;
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
    {"__gc",  lua_script_gc},
    { NULL, NULL }
};

int 
luaopen_Script (lua_State *L)
{
    return lua_meta_open(L, SCRIPT_LIB, script_methods, lua_script_new);
}
