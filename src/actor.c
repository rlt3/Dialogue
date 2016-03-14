#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "actor.h"
#include "script.h"
#include "company.h"
#include "utils.h"

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
    company_set(A);
    dialogue_set_io_write(A);

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
 * An optional argument specifying which Scripts should be forcefully loaded.
 * If one passes the string "all" then all Scripts will be loaded. If one
 * passes an integer, that is the nth Script which will be loaded.
 *
 * Errors from Scripts are caught in sequential order. Meaning an error for the
 * first Script will mask errors for any remaining. Errors are left on top of
 * the Actor's stack and 1 is returned. Otherwise, success, and returns 0.
 */
int
actor_load (Actor *actor, lua_State *L)
{
    const int opt_arg = 2; 
    Script *script = actor->script_head;
    lua_State *A = actor->L;
    int load_index = 0;
    int load_all = 0;
    int ret = 1;
    int i = 1;

    switch (lua_type(L, opt_arg)) {
    case LUA_TNUMBER:
        load_index = lua_tointeger(L, opt_arg);
        break;

    case LUA_TSTRING:
        if (strncmp("all", lua_tostring(L, opt_arg), 3) == 0)
            load_all = 1;
        break;

    default:
        break;
    }

    for (; script != NULL; i++, script = script->next) {
        if (load_index == i)
            script->be_loaded = 1;

        if (load_all || script->be_loaded)
            if (script_load(script, A) != 0)
                goto exit;
    }

    ret = 0;
exit:
    assert(lua_gettop(A) == ret);
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
 *
 * A special Error will occur when an Actor is asked to handle a message with
 * no loaded Scripts.
 */
int
actor_send (Actor *actor, lua_State *L)
{
    lua_State *A = actor->L;
    Script *script = NULL;
    int count = 0;
    int ret = 1;

    luaL_checktype(L, -1, LUA_TTABLE);
    utils_copy_top(A, L);

    for (script = actor->script_head; script != NULL; script = script->next) {
        if (script->is_loaded) {
            count++;
            if (script_send(script, A) != 0)
                goto insert_error;
        }
    }

    if (count == 0) {
        lua_pushfstring(A, "Actor `%d' has no loaded Scripts!", actor->id);
insert_error:
        /* push error string to bottom so message table is popped */
        lua_insert(A, 1);
        goto exit;
    }

    ret = 0;
exit:
    lua_pop(A, 1); /* message table */
    assert(lua_gettop(A) == ret);
    return ret;
}

/*
 * Expects two items on top of L: an integer which is the nth Script of the 
 * Actor and the field of that Script to probe.
 *
 * If the Script could not be found, it returns 1 and leaves an error message
 * on top of the Actor's stack.
 */
int
actor_probe (Actor *actor, lua_State *L)
{
    Script *script = actor->script_head;
    int script_index, i = 1, ret = 1;
    const int script_arg = -2;
    const int field_arg = -1;
    lua_State *A = actor->L;
    const char *field;

    script_index = luaL_checkinteger(L, script_arg);
    field = luaL_checkstring(L, field_arg);
   
    /* loop through the linked-list and while incrementing the counter */
    while (script != NULL && i < script_index) {
        script = script->next;
        i++;
    }

    if (script == NULL || i > script_index) {
        lua_pushfstring(A, "Couldn't find Script @ %d inside Actor `%d`!", 
                script_index, actor->id);
        goto exit;
    }

    /* script_probe leaves an error on A so we ain't gotta do nuthin'!! */
    if (script_probe(script, A, field) != 0)
        goto exit;

    utils_copy_top(L, A);
    lua_pop(A, 1);

    ret = 0;
exit:
    assert(lua_gettop(A) == ret);
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

/*
 * Required for the Tree.h. Sets the id for the Actor. We take this opportunity
 * to push a Lua Actor object for the Scripts to use.
 */
void
actor_assign_id (void *a, int id)
{
    Actor *actor = a;
    actor->id = id;
    company_push_actor(actor->L, id);
    lua_setglobal(actor->L, "actor");
}

/*
 * Required for the Tree.h. Frees the memory at an Actor's pointer.
 */
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
