#include "tmp/actor.h"
#include "tmp/script.h"

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index)
{
    return (Script *) luaL_checkudata(L, index, SCRIPT_LIB);
}

static int
lua_script_new (lua_State *L)
{
    Script *script;
    Actor *actor;
    lua_State *A;

    script_check_table(L, 2);

    actor = lua_check_actor(L, 1);
    lua_State *A = actor_request_stack(actor);
    return 1;
}
