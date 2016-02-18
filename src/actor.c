#include <stdlib.h>
#include "actor.h"
#include "script.h"
#include "utils.h"
#include <assert.h>

struct Actor {
    lua_State *L;
    Script *script_head;
    Script *script_tail;
    int id;
};

void actor_add_script (Actor *actor, Script *script);

/*
 * Pop the error from the Actor onto the given Lua stack.
 */
void
actor_pop_error (Actor *actor, lua_State *L)
{
    utils_copy_top(L, actor->L);
    lua_pop(actor->L, 1);
}

/*
 * Expects the given Lua stack to have the definition at the top of the stack.
 *
 * Definition:
 * { "module name" [, data0 [, data1 [, ... [, dataN]]]] }
 *
 * Examples:
 * { "window", 400, 600 }
 * { "entity", "player.png", 40, 40 }
 * { "entity", "monster.png", 100, 200 }
 *
 * If the return is not NULL, then it is successful.
 */
Actor *
actor_create (lua_State *L)
{
    const int definition_index = lua_gettop(L);
    Script *script = NULL;
    Actor *actor = NULL;
    lua_State *A = NULL;
    int i, len;

    luaL_checktype(L, definition_index, LUA_TTABLE);
    len = luaL_len(L, definition_index);

    actor = malloc(sizeof(*actor));

    if (!actor)
        goto exit;

    A = luaL_newstate();
    
    if (!A) {
        free(actor);
        actor = NULL;
        goto exit;
    }

    luaL_openlibs(A);

    actor->L = A;
    actor->script_head = NULL;
    actor->script_tail = NULL;
    actor->id = -1;

    for (i = 1; i <= len; i++) {
        lua_rawgeti(L, definition_index, i);
        utils_copy_top(A, L);

        script = script_new(A);

        /* 
         * script_new pushes any errors onto A if !script. Fail on creating the
         * Actor if any of its scripts also fail. Note this occurs before any 
         * scripts are loaded because this is the foundation for loading.
         */
        if (!script) {
            for (script = actor->script_head; 
                 script != NULL; 
                 script = script->next)
                script_destroy(script, A);

            utils_copy_top(L, A);

            lua_close(A);
            free(actor);
            actor = NULL;

            lua_error(L);
        }

        actor_add_script(actor, script);
        lua_pop(L, 1);
    }

    assert(lua_gettop(A) == 0);

exit:
    return actor;
}

/*
 * The Actor loads (or reloads) all of its Scripts marked to be loaded.
 *
 * Errors from Scripts are caught in sequential order. Meaning an error for the
 * first Script will mask errors for any remaining. Errors are left on top of
 * the Actor's stack and 1 is returned. Otherwise, success, and returns 0.
 */
int
actor_load (Actor *actor)
{
    lua_State *A = actor->L;
    int ret = 1;
    Script *script;
    for (script = actor->script_head; script != NULL; script = script->next)
        if (script->be_loaded)
            if (script_load(script, A) != 0)
                goto exit;
    ret = 0;
exit:
    assert(lua_gettop(A) == 0);
    return ret;
}

/*
 * The Actor sends the message to all of its Scripts which are loaded.
 *
 * Assumes a message table on top of the given Lua stack in the form of:
 *      { 'message' [, arg1 [, ... [, argn]]], author}
 *
 * The function copies the table from the given Lua stack (L) onto the Actor's
 * Lua stack for all Scripts to access.
 *
 * Errors from Scripts are caught in sequential order. Meaning an error for the
 * first Script will mask errors for any remaining. Errors are left on top of
 * the Actor's stack and 1 is returned. Otherwise, success, and returns 0.
 */
int
actor_send (Actor *actor, lua_State *L)
{
    lua_State *A = actor->L;
    Script *script;
    int ret = 1;

    luaL_checktype(L, -1, LUA_TTABLE);
    utils_copy_top(A, L);

    for (script = actor->script_head; script != NULL; script = script->next)
        if (script->is_loaded)
            if (script_send(script, A) != 0)
                goto exit;
    ret = 0;
exit:
    lua_pop(A, 1);
    assert(lua_gettop(A) == 0);
    return ret;
}

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

void
actor_assign_id (void *actor, int id)
{
    //printf("Actor %p assigned to id %d\n", actor, id);
    ((Actor*)actor)->id = id;
}

int
actor_get_id (void *actor)
{
    return ((Actor*)actor)->id;
}

void
actor_destroy (void *a)
{
    Actor *actor = a;
    Script *script, *next;

    for (script = actor->script_head; script != NULL; script = next) {
	next = script->next;
	script_destroy(script, actor->L);
    }

    actor->script_head = NULL;
    actor->script_tail = NULL;
    lua_close(actor->L);
    free(actor);
}
