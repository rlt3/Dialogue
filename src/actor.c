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
 * Add a child to the given actor, always at the front.
 */
void
actor_add_child (Actor *actor, Actor *child)
{
    if (actor->child == NULL)
        goto set_actor_child;
    child->next = actor->child;
set_actor_child:
    actor->child = child;
    child->parent = actor;
    child->dialogue = actor->dialogue;
    child->mailbox = actor->mailbox;
}

/*
 * From an envelope, send a message to each Script an actor owns.
 */
void
actor_send_envelope (Actor *actor, Envelope *envelope)
{
    Script *script;
    pthread_mutex_lock(&actor->mutex);

    for (script = actor->script; script != NULL; script = script->next) {
        lua_method_push(actor->L, script, SCRIPT_LIB, "send");
        envelope_push_table(actor->L, envelope);
        if (lua_pcall(actor->L, 2, 0, 0))
            luaL_error(actor->L, "Error sending: %s", lua_tostring(actor->L, -1));
    }

    pthread_mutex_unlock(&actor->mutex);
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

    actor->parent = NULL;
    actor->next = NULL;
    actor->child = NULL;
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
 * Create actor from table and add it as a child. Returns the child created.
 * player:child{ {"draw", 2, 4}, { "weapon", "knife" } }
 */
static int
lua_actor_child (lua_State *L)
{
    int table_ref;
    Actor *child, *actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    table_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* Dialogue.Actor{ } */
    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Actor");
    lua_rawgeti(L, LUA_REGISTRYINDEX, table_ref);
    if (lua_pcall(L, 1, 1, 0))
        luaL_error(L, "Creating child failed: %s", lua_tostring(L, -1));

    child = lua_check_actor(L, -1);
    actor_add_child(actor, child);

    return 1;
}

/*
 * Return an array of children an Actor owns.
 */
static int
lua_actor_children (lua_State *L)
{
    int i;
    Actor *child, *actor = lua_check_actor(L, 1);

    lua_newtable(L);

    for (i = 1, child = actor->child; child != NULL; child = child->next, i++) {
        lua_object_push(L, child, ACTOR_LIB);
        lua_rawseti(L, -2, i);
    }

    return 1;
}

/*
 * Return an array of scripts an Actor owns.
 */
static int
lua_actor_scripts (lua_State *L)
{
    int i;
    Script *scpt;
    Actor *actor = lua_check_actor(L, 1);

    lua_newtable(L);

    for (i = 1, scpt = actor->script; scpt != NULL; scpt = scpt->next, i++) {
        lua_object_push(L, scpt, SCRIPT_LIB);
        lua_rawseti(L, -2, i);
    }

    return 1;
}

/*
 * Create an Envelope from a given table and send it to all the Actor's Scripts.
 * player:send{ "move", 0, 1 }
 */
static int
lua_actor_send (lua_State *L)
{
    Envelope *envelope;
    Actor* actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Envelope");
    lua_pushvalue(L, 2);
    lua_call(L, 1, 1);

    envelope = lua_check_envelope(L, -1);
    actor_send_envelope(actor, envelope);
    envelope_free(envelope);

    return 0;
}

static int
lua_actor_tostring (lua_State *L)
{
    Actor* actor = lua_check_actor(L, 1);
    lua_pushfstring(L, "%s %p", ACTOR_LIB, actor);
    return 1;
}

static int
lua_actor_gc (lua_State *L)
{
    Actor* actor = lua_check_actor(L, 1);
    lua_close(actor->L);
    return 0;
}

static const luaL_Reg actor_methods[] = {
    {"child",      lua_actor_child},
    {"children",   lua_actor_children},
    {"give",       lua_actor_give},
    {"scripts",    lua_actor_scripts},
    {"send",       lua_actor_send},
    {"__tostring", lua_actor_tostring},
    {"__gc",       lua_actor_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    return lua_meta_open(L, ACTOR_LIB, actor_methods, lua_actor_new);
}
