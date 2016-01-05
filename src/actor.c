#include <stdio.h>
#include <string.h>
#include "dialogue.h"
#include "actor.h"
#include "script.h"
#include "utils.h"
#include "luaf.h"

lua_State *
actor_request_state (Actor *actor)
{
    return actor->L;
}

void
actor_return_state (Actor *actor)
{
    return;
}

int
actor_is_calling_thread (pthread_t pid)
{
    return 1;
}

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index)
{
    return (Actor *) luaL_checkudata(L, index, ACTOR_LIB);
}

/*
 * Add a child to the end of the Actor's linked-list of children.
 */
void
actor_add_child (Actor *actor, Actor *child)
{
    Actor *sibling;

    if (actor->child == NULL) {
        actor->child = child;
        goto set_actor_child;
    }

    for (sibling = actor->child; child != NULL; sibling = sibling->next) {
        if (sibling->next == NULL) {
            sibling->next = child;
            goto set_actor_child;
        }
    }

set_actor_child:
    child->parent = actor;
    child->dialogue = actor->dialogue;
}

/*
 * Add the Script to the end of the Actor's linked-list of Scripts.
 */
void
actor_add_script (Actor *actor, Script *script)
{
    if (actor->script_head == NULL) {
        actor->script_head = script;
        actor->script_tail = script;
    } else {
        actor->script_tail->next = script;
        script->prev = actor->script_tail;
        actor->script_tail = script;
    }
}

/*
 * Remove the Script from the Actor's linked-list.
 */
void
actor_remove_script (Actor *actor, Script *script)
{
    if (script->prev == NULL && script->next) {
        /* it is the head */
        actor->script_head = script->next;
        actor->script_head->prev = NULL;
    } else if (script->next == NULL && script->prev) {
        /* it is the tail */
        actor->script_tail = script->prev;
        actor->script_tail->next = NULL;
    } else {
        /* a normal node */
        script->prev->next = script->next;
        script->next->prev = script->prev;
    }
}

/*
 * Leave the Lead Actor table on top of the stack. Creates it if it doesn't
 * exist. Will throw an error if not called on Main thread. Returns its
 * position on the stack.
 */
int
actor_lead_table (lua_State *L)
{
    static short int defined = 0;

    lua_getglobal(L, "__main_thread");

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        luaL_error(L, "Lead Actor table only exists in Main thread!");
    } else {
        lua_pop(L, 1);
    }

    if (!defined) {
        lua_newtable(L);
        lua_setglobal(L, "__lead_actor");
        defined = 1;
    }

    lua_getglobal(L, "__lead_actor");
    return lua_gettop(L);
}

/*
 * Put the Actor inside the Lead actor table.
 */
void
actor_assign_lead (Actor *actor, lua_State *L)
{
    const int top = actor_lead_table(L);
    const int len = luaL_len(L, top);

    lua_rawgeti(L, LUA_REGISTRYINDEX, actor->ref);
    lua_rawseti(L, top, len + 1);

    lua_pop(L, 1);
}

/*
 * Create an Actor which has its own thread. It initializes all Scripts and 
 * handles all messages (send/receive) in its own thread because many Lua 
 * modules and objects aren't thread-safe.
 *
 * Create the Actor by sending a table of tables of the Lua module to load and
 * any variables needed to initialize it.
 *
 * Actor{ {"draw", 400, 200}, {"weapon", "longsword"} }
 *
 * Optionally, one can call like Actor{ "Lead", {"draw", 0, 0} }, which creates
 * the Actor as a Lead Actor and loads all of its Scripts here.
 */
static int
lua_actor_new (lua_State *L)
{
    const int table_arg = 1;
    const int actor_arg = 2;
    int script_arg;
    int len = luaL_len(L, table_arg);
    int i, start = 1;
    lua_State *A;
    pthread_mutexattr_t mutex_attr;

    Actor *actor = lua_newuserdata(L, sizeof(*actor));
    luaL_getmetatable(L, ACTOR_LIB);
    lua_setmetatable(L, -2);

    /* 
     * An actor's Action may call one of its own methods, which typically want
     * lock the mutex already locked by doing the action. So, they are 
     * recursive.
     */
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&actor->state_mutex, &mutex_attr);
    pthread_mutex_init(&actor->reference_mutex, &mutex_attr);
    pthread_rwlock_init(&actor->structure, NULL);

    actor->parent = NULL;
    actor->next = NULL;
    actor->child = NULL;
    actor->dialogue = NULL;
    actor->script_head = NULL;
    actor->script_tail = NULL;
    actor->mailbox = NULL;
    actor->reference_count = 0; /* no envelopes or outside structures ref */

    actor->is_lead = 0;
    actor->is_star = 0;

    lua_rawgeti(L, table_arg, start);
    if (lua_type(L, 3) == LUA_TSTRING) {
        start++;
        switch (lua_tostring(L, -1)[0]) {
        case 'S':
            actor->is_star = 1;
            break;
        case 'L':
            actor->is_lead = 1;
            break;
        default:
            break;
        }
    }
    lua_pop(L, 1);

    actor->L = luaL_newstate();
    A = actor->L;
    luaL_openlibs(A);

    /* load this module (the one you're reading) into the Actor's state */
    luaL_requiref(A, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(A, 1);

    /* push Actor so Scripts can reference the Actor it belongs to. */
    utils_push_object(A, actor, ACTOR_LIB);
    lua_setglobal(A, "actor");

    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Actor");
    lua_getfield(L, -1, "Script");
    script_arg = lua_gettop(L);

    for (i = start; i < len; i++) {
        lua_getfield(L, script_arg, "new");
        lua_pushvalue(L, actor_arg);
        lua_rawgeti(L, table_arg, i);
        lua_call(L, 2, 1);
        lua_pop(L, 1);
    }
    lua_pop(L, 3);

    /* luaf(L, "Dialogue.Post.send(%2, 'load')"); */

    return 1;
}

static int
lua_actor_scripts (lua_State *L)
{
    int i = 0;
    Script *script;
    Actor *actor = lua_check_actor(L, -1);

    actor_request_state(actor);
    lua_newtable(L);
    for (script = actor->script_head; script != NULL; script = script->next) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, script->ref);
        lua_rawseti(L, -2, ++i);
    }
    actor_return_state(actor);

    return luaf(L, "return Collection(%2)", 1); 
}

static int
lua_actor_gc (lua_State *L)
{
    Actor* actor = lua_check_actor(L, 1);
    lua_close(actor->L);
    return 0;
}

static int
lua_actor_tostring (lua_State *L)
{
    Actor* actor = lua_check_actor(L, 1);
    lua_pushfstring(L, "%s %p", ACTOR_LIB, actor);
    return 1;
}

static const luaL_Reg actor_methods[] = {
    /*{"audience",   lua_actor_audience},*/
    {"scripts",    lua_actor_scripts},
    {"__gc",       lua_actor_gc},
    {"__tostring", lua_actor_tostring},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    utils_lua_meta_open(L, ACTOR_LIB, actor_methods, lua_actor_new);
    
    luaL_requiref(L, SCRIPT_LIB, luaopen_Dialogue_Actor_Script, 1);
    lua_setfield(L, -2, "Script");

    //luaL_requiref(L, MAILBOX_LIB, luaopen_Dialogue_Actor_Mailbox, 1);
    //lua_setfield(L, -2, "Mailbox");

    return 1;
}
