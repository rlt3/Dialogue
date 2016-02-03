#include <stdlib.h>
#include "actor.h"
#include "utils.h"

struct Actor {
    lua_State *L;
    int is_lead; /* restrict to a single thread */
    int is_star; /* restrict to the main thread */
    int id;      /* id inside the Company */
};

Actor *
actor_create (lua_State *L)
{
    Actor *actor = NULL;
    lua_State *A = NULL;
    //const int definition_index = lua_gettop(L);

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
    actor->is_lead = 0;
    actor->is_star = 0;
    actor->id = -1;

    utils_copy_top(A, L);
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
