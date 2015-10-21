#include <stdio.h>
#include "actor.h"
#include "script.h"
#include "mailbox.h"
#include "utils.h"

/* 
 * A Dialogue is a table that follows this form: { {}, {} }
 *
 * The first element of the table is a table that holds an Actor's scripts. The
 * second element is a table that holds the children of that Actor.
 *
 * Dialogue{ {},
 *     {
 *         { { {"draw", 400, 200} }, {} }
 *         { { {"draw", 2, 4} }, {} }
 *     }
 * }
 */
int
lua_dialogue_new (lua_State *L)
{
    Actor *actor, *child;
    luaL_checktype(L, 1, LUA_TTABLE);  /* 1 */

    /* push the Scripts part of the table to create an Actor */
    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Actor");
    table_push_head(L, 1);
    lua_call(L, 1, 1);
    actor = lua_check_actor(L, -1);
    /* pull reference to entire userdata to push later */
    actor->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);

    /*
     * The recursion relies on sending the first created Actor (the head) to
     * all the descendants as a second parameter. So, every Dialogue.new call
     * past the first will be called like Dialogue.new(table, head).
     */
    if (lua_gettop(L) == 1) {
        actor->dialogue = actor;
        lua_getglobal(L, "Dialogue");
        lua_getfield(L, -1, "Mailbox");
        lua_call(L, 0, 1);
        actor->mailbox = lua_check_mailbox(L, -1);
        actor->mailbox->ref = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_pop(L, 1);
    } else {
        actor->dialogue = lua_check_actor(L, 2);
        lua_pop(L, 1);
    }

    /* push the children part of the table and recurse */
    lua_rawgeti(L, 1, 2);
    lua_pushnil(L);
    while (lua_next(L, 2)) {
        lua_getglobal(L, "Dialogue");
        lua_getfield(L, -1, "new");
        lua_pushvalue(L, -3);
        lua_object_push(L, actor->dialogue, ACTOR_LIB);
        lua_call(L, 2, 1);

        child = lua_check_actor(L, -1);
        actor_add_child(actor, child);

        lua_pop(L, 3); /* child, Dialogue table, and table value */
    }
    lua_pop(L, 1);

    /* push the reference to the userdata (instead of pushing light userdata) */
    lua_rawgeti(L, LUA_REGISTRYINDEX, actor->ref);

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
    
    luaL_requiref(L, MAILBOX_LIB, luaopen_Dialogue_Mailbox, 1);
    lua_setfield(L, -2, "Mailbox");

    lua_pushcfunction(L, lua_dialogue_new);
    lua_setfield(L, -2, "new");

    return 1;
}
