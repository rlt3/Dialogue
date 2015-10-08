#include "actor.h"
#include "script.h"
#include "envelope.h"

Dialogue *
lua_check_dialogue (lua_State *L, int index)
{
    return lua_checkactor(L, index);
}

int
dialogue_new (lua_State *L)
{
    Actor *actor, *child;
    luaL_checktype(L, 1, LUA_TTABLE);  /* 1 */

    /* push the Scripts part of the table to create an Actor */
    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Actor");
    lua_push_head(L, 1);
    if (lua_call(L, 1, 1, 0))
        luaL_error(L, "Failed to create Actor: %s", lua_tostring(L, -1));

    actor = lua_check_actor(L, 2); /* 2 */

    /* push the second part of the table -- the children */
    lua_rawgeti(L, 1, 2);  /* 3 */

    /* Create and add all the children */
    lua_pushnil(L);
    while (lua_next(L, 3)) { /* table @ 6, key @ 5 */

        table_push_head(L, 6);  /* 7 */
        lua_pushnil(L);
        while (lua_next(L, 7)) { /* table @ 10, key @ 9 */

            lua_method_push(L, actor, ACTOR_LIB, "child"); /* 11 & 12 */
            lua_pushvalue(L, 10);
            if (lua_call(L, 2, 1, 0))
                luaL_error(L, "Failed to descend Dialogue children: %s", 
                        lua_tostring(L, -1));

            child = lua_check_actor(L, -1);

            lua_pop(L, 2); /* pop table and return value */
        }
        lua_pop(L, 2); /* pop nil and table head */

        lua_getglobal(L, "Dialogue");
        lua_getfield(L, -1, "new");
        lua_rawgeti(L, 6, 2);
        if (lua_call(L, 2, 1, 0))
            luaL_error(L, "Failed to descend Dialogue children: %s", 
                    lua_tostring(L, -1));

        lua_pop(L, 1);
    }

    lua_pop(L, 2); /* pop the nil key and table head */

    /* 
     * A dialogue is the head of a tree. All actors in a dialogue inherit 
     * certain properties. These are those properties.
     */
    actor->mailbox = box;
    actor->dialogue = actor;
    actor->parent = actor;
    lua_pushobject(L, ACTOR_LIB);

    return 1;
}


int
luaopen_Dialogue (lua_State *L)
{
    lua_newtable(L);

    luaL_requiref(L, ENVELOPE_LIB, luaopen_Dialogue_Envelope, 1);
    lua_setfield(L, -2, "Envelope");

    luaL_requiref(L, ACTOR_LIB, luaopen_Dialogue_Actor, 1);
    lua_setfield(L, -2, "Actor");

    luaL_requiref(L, SCRIPT_LIB, luaopen_Dialogue_Actor_Script, 1);
    lua_setfield(L, -2, "Script");

    return 1;
}
