#include <stdio.h>
#include "actor.h"
#include "envelope.h"
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
//int
//lua_dialogue_new (lua_State *L)
//{
//    Actor *actor, *child;
//    int dialogue_table = 1;
//    int thread_count = 8;
//    int children_table;
//    int args = lua_gettop(L);
//
//    luaL_checktype(L, dialogue_table, LUA_TTABLE);
//
//    /* push the Scripts part of the table to create an Actor */
//    lua_getglobal(L, "Dialogue");
//    lua_getfield(L, -1, "Actor");
//    lua_getfield(L, -1, "new");
//    utils_push_table_head(L, dialogue_table);
//    lua_call(L, 1, 1);
//    actor = lua_check_actor(L, -1);
//    actor->ref = luaL_ref(L, LUA_REGISTRYINDEX);
//    lua_pop(L, 2);
//
//    /*
//     * The recursion relies on sending the first created Actor (the head) to
//     * all the descendants as a second parameter. So, every Dialogue.new call
//     * past the first will be called like Dialogue.new(table, head).
//     */
//    if (args == 1) {
//        actor->dialogue = actor;
//        actor->mailbox = NULL;
//        lua_getglobal(L, "Dialogue");
//        lua_getfield(L, -1, "Mailbox");
//        lua_getfield(L, -1, "new");
//        lua_pushinteger(L, thread_count);
//        lua_call(L, 1, 1);
//        actor->mailbox = lua_check_mailbox(L, -1);
//        actor->mailbox->ref = luaL_ref(L, LUA_REGISTRYINDEX);
//        lua_pop(L, 2);
//    } else {
//        actor->dialogue = lua_check_actor(L, 2);
//        actor->mailbox = actor->dialogue->mailbox;
//        lua_pop(L, 1);
//    }
//
//    /* push the children part of the table and recurse */
//    lua_rawgeti(L, dialogue_table, 2);
//    children_table = lua_gettop(L);
//    lua_pushnil(L);
//    while (lua_next(L, children_table)) {
//        lua_getglobal(L, "Dialogue");
//        lua_getfield(L, -1, "new");
//        lua_pushvalue(L, -3);
//        lua_rawgeti(L, LUA_REGISTRYINDEX, actor->dialogue->ref);
//        lua_call(L, 2, 1);
//
//        child = lua_check_actor(L, -1);
//        actor_add_child(actor, child);
//
//        lua_pop(L, 3); /* child, Dialogue table, and table value */
//    }
//    lua_pop(L, 1);
//
//    /* push the reference to the userdata (instead of pushing light userdata) */
//    lua_rawgeti(L, LUA_REGISTRYINDEX, actor->ref);
//
//    return 1;
//}

int
luaopen_Dialogue (lua_State *L)
{
    int t_index;

    lua_newtable(L);
    t_index = lua_gettop(L);

    luaL_requiref(L, ACTOR_LIB, luaopen_Dialogue_Actor, 1);
    lua_setfield(L, t_index, "Actor");

    lua_getfield(L, t_index, "Actor");
    luaL_requiref(L, ACTOR_LIB, luaopen_Dialogue_Actor_Script, 1);
    lua_setfield(L, -2, "Script");
    lua_pop(L, 1);
    
    //luaL_requiref(L, MAILBOX_LIB, luaopen_Dialogue_Mailbox, 1);
    //lua_setfield(L, t_index, "Mailbox");

    //lua_getfield(L, t_index, "Mailbox");
    //luaL_requiref(L, ENVELOPE_LIB, luaopen_Dialogue_Envelope, 1);
    //lua_setfield(L, -2, "Envelope");
    //lua_pop(L, 1);

    //lua_pushcfunction(L, lua_dialogue_new);
    //lua_setfield(L, t_index, "new");

    return 1;
}
