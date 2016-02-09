#include <stdlib.h>
#include "actor.h"
#include "utils.h"

struct Actor {
    lua_State *L;
    int id;
};

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
    actor->id = -1;

    for (i = 1; i <= len; i++) {
        lua_rawgeti(L, definition_index, i);
        printf("%s ", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

exit:
    return actor;
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
    //printf("Destroying Actor %p with id %d\n", a, actor->id);
    lua_close(actor->L);
    free(actor);
}
