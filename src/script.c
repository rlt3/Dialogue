#include "script.h"

struct Script {
    int table_reference;
    Script *prev;
    Script *next;
};

/*
 * Create a new Script from a module and return it.
 * Script("collision") => script{module = "collision"}
 */
int
lua_script_new (lua_State *L)
{
    const char *module_name = luaL_checkstring(L, 1);
    Script *script = lua_newuserdata(L, sizeof(Script));
    script->prev = NULL;
    script->next = NULL;

    /* push our Script userdata on top of the stack as an object */
    luaL_getmetatable(L, SCRIPT_LIB);
    lua_setmetatable(L, -2);
    return 1;
}

/*
 * In the given lua_State (the Actor's most likely), send the message inside
 * the envelope reference.
 */
int
script_send (Script *script, lua_State *L, Envelope *envelope);
