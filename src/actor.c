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
 * Actor{ {"draw", 400, 200}, {"weapon", "longsword"} };
 */
static int
lua_actor_new (lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);  /* 1 */
    Actor *actor = lua_newuserdata(L, sizeof(Actor)); /* 2 */
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
    lua_object_push(actor->L, actor, ACTOR_LIB);
    lua_setglobal(actor->L, "actor");

    /* call actor:give on each sub-table in this table of tables */
    lua_pushnil(L);
    while (lua_next(L, 1)) {
        luaL_checktype(L, -1, LUA_TTABLE);

        lua_getfield(L, 2, "give");
        lua_object_push(L, actor, ACTOR_LIB);
        lua_pushvalue(L, -3);
        if (lua_pcall(L, 2, 0, 0))
            luaL_error(L, "Creating Actor failed: %s", lua_tostring(L, -1));

        lua_pop(L, 1);
    }

    return 1;
}

/*
 * Create a Script from a given table and 'give' it to the actor.
 * player:give{ "weapon", "gun" }
 */
static int
lua_actor_give (lua_State *L)
{
    Script *script = NULL;
    Actor* actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    /* Dialogue.Script{ } */
    lua_getglobal(actor->L, "Dialogue");
    lua_getfield(actor->L, -1, "Script");
    table_push_copy(L, actor->L, 2);
    if (lua_pcall(actor->L, 1, 1, 0))
        luaL_error(L, "Giving script failed: %s", lua_tostring(actor->L, -1));

    script = lua_check_script(actor->L, -1);
    actor_add_script(actor, script);

    /* script:load() */
    lua_getfield(actor->L, -1, "load");
    lua_object_push(actor->L, script, SCRIPT_LIB);
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
    int message_ref;
    Script *script = NULL;
    Actor* actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    pthread_mutex_lock(&actor->mutex);

    /* copy and immediately pop into a ref so we can continually use it */
    table_push_copy(L, actor->L, 2);
    message_ref = luaL_ref(actor->L, LUA_REGISTRYINDEX);
    
    for (script = actor->script; script != NULL; script = script->next) {
        lua_method_push(actor->L, script, SCRIPT_LIB, "send");
        lua_rawgeti(actor->L, LUA_REGISTRYINDEX, message_ref);
        if (lua_pcall(actor->L, 2, 0, 0))
            luaL_error(L, "Sending message failed: %s", lua_tostring(actor->L, -1));
    }

    luaL_unref(actor->L, LUA_REGISTRYINDEX, message_ref);

    /* TODO: error conditions affect this how? */
    pthread_mutex_unlock(&actor->mutex);

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
