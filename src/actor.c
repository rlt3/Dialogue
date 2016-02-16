#include <stdlib.h>
#include "actor.h"
#include "script.h"
#include "utils.h"

struct Actor {
    lua_State *L;
    Script *script_head;
    Script *script_tail;
    int id;
};

void actor_add_script (Actor *actor, Script *script);

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
            for (script = actor->script_head; script != NULL; script = script->next)
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

exit:
    return actor;
}

/*
 * To avoid longjmps from the Actors' Lua stacks, we pop any error message off 
 * them and onto the global (or calling) Lua state and error from there.
 */
void
script_error (lua_State *L, lua_State *A)
{
    utils_copy_top(L, A);
    lua_pop(A, 1);
    luaL_checktype(L, -1, LUA_TSTRING);
    lua_error(L);
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
    Script *script = NULL;
    Actor *actor = a;

    //printf("Destroying Actor %p with id %d\n", a, actor->id);

    for (script = actor->script_head; script != NULL; script = script->next)
        script_destroy(script, actor->L);

    lua_close(actor->L);
    free(actor);
}
