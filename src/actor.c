#include <stdio.h>
#include "dialogue.h"
#include "actor.h"
#include "script.h"
#include "utils.h"

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index)
{
    return (Actor *) luaL_checkudata(L, index, ACTOR_LIB);
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
 * Create an Actor which has its own thread. It initializes all Scripts and 
 * handles all messages (send/receive) in its own thread because many Lua 
 * modules and objects aren't thread-safe.
 *
 * Create the Actor by sending a table of tables of the Lua module to load and
 * any variables needed to initialize it.
 *
 * Actor{ {"draw", 400, 200}, {"weapon", "longsword"} }
 */
static int
lua_actor_new (lua_State *L)
{
    const int table_arg = 1;
    const int actor_arg = 2;
    int table_index;
    int script_index;
    lua_State *A;
    Actor *actor;
    Script *script;
    pthread_mutexattr_t mutex_attr;

    luaL_checktype(L, table_arg, LUA_TTABLE);

    actor = lua_newuserdata(L, sizeof(Actor));
    luaL_getmetatable(L, ACTOR_LIB);
    lua_setmetatable(L, -2);

    actor->parent = NULL;
    actor->next = NULL;
    actor->child = NULL;
    actor->script_head = NULL;
    actor->script_tail = NULL;
    actor->mailbox = NULL;
    actor->dialogue = NULL;

    actor->new_action = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    actor->action = LOAD;
    actor->on = 1;

    /* 
     * init mutexes to recursive because its own thread might call a method
     * which expects to be called from an outside thread sometimes and askes
     * for a mutex.
     */
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&actor->state_mutex, &mutex_attr);

    actor->L = luaL_newstate();
    A = actor->L;
    luaL_openlibs(A);

    /* load this module (the one you're reading) into the Actor's state */
    luaL_requiref(A, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(A, 1);

    /* push Actor so Scripts can reference the Actor it belongs to. */
    utils_push_object(A, actor, ACTOR_LIB);
    lua_setglobal(A, "actor");

    /* make a table for envelopes to sit in */
    lua_newtable(A);
    lua_setglobal(A, "__envelopes");

    /* Push the Script table to prepare for calling 'new' a bunch */
    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Actor");
    lua_getfield(L, -1, "Script");
    script_index = lua_gettop(L);

    /* Create all the Scripts in this Lua state */
    lua_pushnil(L);
    while (lua_next(L, table_arg)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        table_index = lua_gettop(L);

        lua_getfield(L, script_index, "new");
        lua_pushvalue(L, actor_arg);
        lua_pushvalue(L, table_index);

        if (lua_pcall(L, 2, 1, 0))
            luaL_error(L, "Giving script failed: %s", lua_tostring(L, -1));

        script = lua_check_script(L, -1);
        script->ref = luaL_ref(L, LUA_REGISTRYINDEX);
        actor_add_script(actor, script);

        lua_pop(L, 1); /* Key */
    }
    lua_pop(L, 3); /* Dialogue, Actor, Script */

    pthread_create(&actor->thread, NULL, actor_thread, actor);
    pthread_detach(actor->thread);

    return 1;
}

/*
 * This is a blocking method. It puts the the given message inside the 
 * Envelopes table and tells the Actor to process it.
 */
static int
lua_actor_send (lua_State *L)
{
    lua_State *A;
    Actor *actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    A = actor_request_state(actor);
    lua_getglobal(A, "__envelopes");
    utils_copy_top(A, L);
    lua_rawseti(A, 1, luaL_len(A, 1) + 1);
    lua_pop(A, 1);
    actor_return_state(actor);

    actor_alert_action(actor, RECEIVE);
    
    return 0;
}

/*
 * This is a blocking method. Returns an array of Script references of an actor.
 */
static int
lua_actor_scripts (lua_State *L)
{
    int i = 0;
    Script *script;
    Actor *actor = lua_check_actor(L, 1);

    actor_request_state(actor);
    lua_newtable(L);
    for (script = actor->script_head; script != NULL; script = script->next) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, script->ref);
        lua_rawseti(L, -2, ++i);
    }
    actor_return_state(actor);

    return 1;
}

/*
 * Set the thread's condition to false and close the Lua stack.
 */
static int
lua_actor_gc (lua_State *L)
{
    lua_State *A;
    Script *script;
    Actor *actor = lua_check_actor(L, 1);

    actor_alert_action(actor, STOP);
    usleep(1000);

    A = actor_request_state(actor);
    for (script = actor->script_head; script != NULL; script = script->next)
        script_unload(script);
    lua_close(A);
    actor_return_state(actor);

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
    {"send",       lua_actor_send},
    {"scripts",    lua_actor_scripts},
    {"__gc",       lua_actor_gc},
    {"__tostring", lua_actor_tostring},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    return utils_lua_meta_open(L, ACTOR_LIB, actor_methods, lua_actor_new);
}
