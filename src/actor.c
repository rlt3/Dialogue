#include <pthread.h>
#include "dialogue.h"
#include "envelope.h"
#include "actor.h"
#include "script.h"
#include "utils.h"

struct Actor {
    lua_State *L;
    Script *script;
    pthread_mutex_t mutex;
};

/*
 * Add a script to the given actor, always at the front.
 */
void
actor_add_script (Actor *actor, Script *script)
{
    if (actor->script == NULL)
        goto set_actor_script;
    script->next = actor->script;
set_actor_script:
    actor->script = script;
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
 * Create an Actor, which is a glorified lua_State that holds specific scripts.
 */
static int
lua_actor_new (lua_State *L)
{
    Actor *actor = lua_newuserdata(L, sizeof(Actor));
    luaL_getmetatable(L, ACTOR_LIB);
    lua_setmetatable(L, -2);

    actor->script = NULL;
    actor->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    actor->L = luaL_newstate();
    luaL_openlibs(actor->L);

    /* load this module (the one you're reading) into the Actor's state */
    luaL_requiref(actor->L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(actor->L, 1);

    /* push Actor so Scripts can reference the Actor it belongs to. */
    lua_pushlightuserdata(actor->L, actor);
    luaL_getmetatable(actor->L, ACTOR_LIB);
    lua_setmetatable(actor->L, -2);
    lua_setglobal(actor->L, "actor");

    return 1;
}

/*
 * Create a Script from a given table and 'give' it to the actor.
 * player:give{ "weapon", "gun" }
 */
static int
lua_actor_give (lua_State *L)
{
    Actor* actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    return 0;
}

/*
 * Create an Envelope from a given table and send it to all the Actor's Scripts.
 * player:send{ "move", 0, 1 }
 */
static int
lua_actor_send (lua_State *L)
{
    return 0;
}

static int
lua_actor_gc (lua_State *L)
{
    Actor* actor = lua_check_actor(L, 1);
    lua_close(actor->L);
    return 0;
}

static const luaL_Reg actor_methods[] = {
    {"give", lua_actor_give},
    {"send", lua_actor_send},
    {"__gc", lua_actor_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    return lua_meta_open(L, ACTOR_LIB, actor_methods, lua_actor_new);
}
