#include "dialogue.h"
#include "envelope.h"
#include "actor.h"
#include "script.h"
#include "utils.h"

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
    int i;
    Script *script = NULL;
    Actor* actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    /* Dialogue.Script{ } */
    lua_getglobal(actor->L, "Dialogue");
    lua_getfield(actor->L, -1, "Script");
    lua_newtable(actor->L);

    lua_pushnil(L);
    /* notice we are looping through a table in L, to be put into actor->L */
    for (i = 1; lua_next(L, 2); i++) {
        lua_pushstring(actor->L, lua_tostring(L, -1));
        lua_rawseti(actor->L, -2, i);
        lua_pop(L, 1);
    }
    
    if (lua_pcall(actor->L, 1, 1, 0))
        luaL_error(L, "Giving script failed: %s", lua_tostring(actor->L, -1));

    script = lua_check_script(actor->L, -1);
    actor_add_script(actor, script);

    lua_getfield(actor->L, -1, "load");
    script_push(actor->L, script);
    if (lua_pcall(actor->L, 1, 0, 0))
        luaL_error(L, "Script failed to load: %s", lua_tostring(actor->L, -1));

    return 0;
}

/*
 * Create an Envelope from a given table and send it to all the Actor's Scripts.
 * player:send{ "move", 0, 1 }
 */
static int
lua_actor_send (lua_State *L)
{
    int i, envelope_ref;
    Script *script = NULL;
    Actor* actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    /* Dialogue.Envelope{ } */
    lua_getglobal(actor->L, "Dialogue");
    lua_getfield(actor->L, -1, "Envelope");
    lua_newtable(actor->L);

    lua_pushnil(L);
    /* notice we are looping through a table in L, to be put into actor->L */
    for (i = 1; lua_next(L, 2); i++) {
        lua_pushstring(actor->L, lua_tostring(L, -1));
        lua_rawseti(actor->L, -2, i);
        lua_pop(L, 1);
    }

    if (lua_pcall(actor->L, 1, 1, 0))
        luaL_error(L, "Loading envelope failed: %s", lua_tostring(actor->L, -1));

    envelope_ref = luaL_ref(actor->L, LUA_REGISTRYINDEX);

    /* Use the envelope and send it to each of the Scripts */
    for (script = actor->script; script != NULL; script = script->next) {
        script_push(actor->L, script);
        lua_getfield(actor->L, -1, "send");
        script_push(actor->L, script);
        lua_rawgeti(actor->L, LUA_REGISTRYINDEX, envelope_ref);
        if (lua_pcall(actor->L, 2, 0, 0))
            luaL_error(L, "Sending message failed: %s", lua_tostring(actor->L, -1));
    }

    luaL_unref(actor->L, LUA_REGISTRYINDEX, envelope_ref);

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
