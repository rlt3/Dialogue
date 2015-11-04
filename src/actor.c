#include <stdio.h>
#include "dialogue.h"
#include "actor.h"
#include "script.h"
#include "mailbox.h"
#include "envelope.h"
#include "post.h"
#include "tone.h"
#include "utils.h"

/*
 * Request the stack from the given actor so that we can have thread-safe apis.
 */
lua_State*
actor_request_stack (Actor *actor)
{
    //pthread_mutex_lock(&actor->stack_mutex);
    return actor->L;
}

/*
 * Return the stack back to the Actor who can give it out again.
 */
void
actor_return_stack (Actor *actor)
{
    //pthread_mutex_unlock(&actor->stack_mutex);
}

/*
 * Add the Script to the end of the Actor's linked-list of Scripts.
 */
void
actor_add_script (Actor *actor, Script *new_script)
{
    Script *script;

    if (actor->script == NULL) {
        actor->script = new_script;
        return;
    }

    for (script = actor->script; script != NULL; script = script->next) {
        if (script->next == NULL) {
            script->next = new_script;
            return;
        }
    }
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
    child->mailbox = actor->mailbox;
}

/*
 * Find and remove the Script from the Actor's linked-list of Scripts.
 */
void
actor_remove_script (Actor *actor, Script *removing)
{
    Script *now, *previous = NULL;

    if (actor->script == removing)
        actor->script = NULL;
    
    for (now = actor->script; now != NULL; previous = now, now = now->next) {
        if (now != removing)
            continue;

        previous->next = now->next;
        break;
    }
}

/*
 * From an envelope, send a message to each Script an actor owns.
 */
void
actor_send_envelope (Actor *actor, Envelope *envelope)
{
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
    int actor_ref;
    lua_State *A;
    Actor *actor;
    luaL_checktype(L, 1, LUA_TTABLE);

    actor = lua_newuserdata(L, sizeof(Actor));
    luaL_getmetatable(L, ACTOR_LIB);
    lua_setmetatable(L, -2);

    actor->parent = NULL;
    actor->next = NULL;
    actor->child = NULL;
    actor->script = NULL;
    actor->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    actor->stack_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    actor->L = luaL_newstate();
    A = actor->L;

    /* we create a reference so that our actor doesn't get gc'd during setup */
    actor_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    luaL_openlibs(A);

    /* load this module (the one you're reading) into the Actor's state */
    luaL_requiref(A, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(A, 1);

    /* push Actor so Scripts can reference the Actor it belongs to. */
    utils_push_object(A, actor, ACTOR_LIB);
    lua_setglobal(A, "actor");

    /* Set the scripts from the table passed in */
    lua_rawgeti(L, LUA_REGISTRYINDEX, actor_ref);
    lua_getfield(L, 2, "scripts");
    lua_rawgeti(L, LUA_REGISTRYINDEX, actor_ref);
    lua_pushvalue(L, 1);
    lua_call(L, 2, 1);
    lua_pop(L, 1);

    luaL_unref(L, LUA_REGISTRYINDEX, actor_ref);

    return 1;
}

/*
 * Return a list of Actors which are the audience filtered by the tone given as
 * a string.
 * actor:audience("say") => { actor, actor, ... }
 * actor:audience("command") => { child, child, ... }
 */
static int
lua_actor_audience (lua_State *L)
{
    Actor *actor = lua_check_actor(L, 1);
    const char *tone = luaL_checkstring(L, 2);
    tone_filter(L, actor, tone);
    return 1;
}

/*
 * From a given table, create a Script, give it to the actor, then load it.
 * Returns the loaded Script.
 * player:give{ "weapon", "gun" }
 */
static int
lua_actor_give (lua_State *L)
{
    lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Actor");
    lua_getfield(L, -1, "Script");
    lua_getfield(L, -1, "new");
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    if (lua_pcall(L, 2, 1, 0))
        luaL_error(L, "Giving script failed: %s", lua_tostring(L, -1));

    lua_getfield(L, -1, "load");
    lua_pushvalue(L, -2);
    if (lua_pcall(L, 1, 0, 0))
        luaL_error(L, "Script failed to load: %s", lua_tostring(L, -1));

    return 1;
}

/*
 * An optional table of Scripts (table of tables) can be given which tells the
 * method to purge all current Scripts and replace them. Or the method can be
 * called with no arguments. In either case, a list of Scripts is returned.
 *
 * actor:scripts() => { script, ... }
 * actor:scripts{ {"weapon", "axe", "up"} } => { script }
 */
static int
lua_actor_scripts (lua_State *L)
{
    int i, args = lua_gettop(L);
    Script *s;
    Actor *actor = lua_check_actor(L, 1);

    if (args == 1)
        goto list_return;

    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getfield(L, 1, "drop");
    utils_push_object(L, actor, ACTOR_LIB);
    lua_call(L, 1, 1);
    lua_pop(L, 1);

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_getfield(L, 1, "give");
        utils_push_object(L, actor, ACTOR_LIB);
        lua_pushvalue(L, -3);
        lua_call(L, 2, 1);
        lua_pop(L, 2);
    }

list_return:
    lua_newtable(L);

    for (i = 1, s = actor->script; s != NULL; s = s->next, i++) {
        utils_push_object(L, s, SCRIPT_LIB);
        lua_rawseti(L, -2, i);
    }

    return 1;
}

/*
 * Drop all of the scripts currently owned. Returns the number scripts dropped.
 */
static int
lua_actor_drop (lua_State *L)
{
    int i = 0;
    Actor *actor = lua_check_actor(L, 1);
    Script *script;
    
    for (script = actor->script; script != NULL; script = script->next, i++)
        luaL_unref(actor->L, LUA_REGISTRYINDEX, script->ref);

    actor->script = NULL;
    lua_pushinteger(L, i);
    return 1;
}

/*
 * Create actor from table and add it as a child. Returns the child created.
 * player:child{ {"draw", 2, 4}, { "weapon", "knife" } }
 */
static int
lua_actor_child (lua_State *L)
{
    Actor *child, *actor = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Actor");
    lua_getfield(L, -1, "new");
    lua_pushvalue(L, 2);
    if (lua_pcall(L, 1, 1, 0))
        luaL_error(L, "Creating child failed: %s", lua_tostring(L, -1));

    child = lua_check_actor(L, -1);
    actor_add_child(actor, child);

    child->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_rawgeti(L, LUA_REGISTRYINDEX, child->ref);

    return 1;
}

/*
 * An optional table of Actors (table of tables) can be given which tells the
 * method to abandon its current children and create new ones from table. Or
 * the method can be called with no arguments. In either case, a list of
 * Actors is returned.
 *
 * actor:children() => { script, ... }
 * actor:children{ { {"weapon", "axe", "up"} } } => { Actor }
 */
static int
lua_actor_children (lua_State *L)
{
    int i, args = lua_gettop(L);
    Actor *child;
    Actor *actor = lua_check_actor(L, 1);

    if (args == 1)
        goto list_return;

    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getfield(L, 1, "abandon");
    utils_push_object(L, actor, ACTOR_LIB);
    lua_call(L, 1, 1);
    lua_pop(L, 1);

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_getfield(L, 1, "child");
        utils_push_object(L, actor, ACTOR_LIB);
        lua_pushvalue(L, -3);
        lua_call(L, 2, 1);
        lua_pop(L, 2);
    }

list_return:
    lua_newtable(L);

    for (i = 1, child = actor->child; child != NULL; child = child->next, i++) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, child->ref);
        lua_rawseti(L, -2, i);
    }

    return 1;
}

/*
 * Remove all the children that an Actor has created. Returns the number of 
 * children abandoned (you sicko).
 */
static int
lua_actor_abandon (lua_State *L)
{
    int i = 0;
    Actor *parent = lua_check_actor(L, 1);
    Actor *actor;
    
    for (actor = parent->child; actor != NULL; actor = actor->next, i++)
        luaL_unref(L, LUA_REGISTRYINDEX, actor->ref);

    parent->child = NULL;
    lua_pushinteger(L, i);
    return 1;
}

static int
lua_actor_think (lua_State *L)
{
    return 0;
}

static int
lua_actor_yell (lua_State *L)
{
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
    utils_push_object_method(L, actor, ACTOR_LIB, "drop");
    lua_call(L, 1, 1);
    lua_pop(L, 1);
    lua_close(actor->L);
    luaL_unref(L, LUA_REGISTRYINDEX, actor->ref);
    return 0;
}

static const luaL_Reg actor_methods[] = {
    {"give",       lua_actor_give},
    {"scripts",    lua_actor_scripts},
    {"drop",       lua_actor_drop},
    {"child",      lua_actor_child},
    {"children",   lua_actor_children},
    {"abandon",    lua_actor_abandon},
    {"audience",   lua_actor_audience},
    {"send",       lua_actor_think},
    {"think",      lua_actor_think},
    {"yell",       lua_actor_yell},
    {"__tostring", lua_actor_tostring},
    {"__gc",       lua_actor_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    return utils_lua_meta_open(L, ACTOR_LIB, actor_methods, lua_actor_new);
}
