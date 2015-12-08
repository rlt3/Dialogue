#include <stdio.h>
#include "actor.h"
#include "envelope.h"
#include "script.h"
#include "mailbox.h"
#include "utils.h"

/*
 * Set a Lead actor in the Dialogue. This causes that Actor to be processed on
 * the main thread with the interpreter.
 */
int
lua_dialogue_lead (lua_State *L)
{
    //Actor *actor = lua_check_actor(L, 1);

    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "__lead_actors");

    return 0;
}

int
luaopen_Dialogue (lua_State *L)
{
    int t_index;

    lua_newtable(L);
    t_index = lua_gettop(L);

    luaL_requiref(L, ACTOR_LIB, luaopen_Dialogue_Actor, 1);
    lua_setfield(L, t_index, "Actor");

    lua_pushcfunction(L, lua_dialogue_lead);
    lua_setfield(L, t_index, "lead");

    lua_newtable(L);
    lua_setfield(L, t_index, "__lead_actors");

    //lua_pushcfunction(L, lua_dialogue_new);
    //lua_setfield(L, t_index, "new");

    return 1;
}
