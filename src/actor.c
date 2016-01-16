#include <stdlib.h>
#include "actor.h"
#include "utils.h"

Actor *
actor_create (lua_State *L, int id)
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
        goto exit;
    }

    luaL_openlibs(A);

    actor->L = A;
    actor->is_lead = 0;
    actor->is_star = 0;
    actor->id = id;
    pthread_mutex_init(&actor->mutex, NULL);

    utils_copy_top(A, L);

    printf("Creating Actor %d\n", actor->id);

exit:
    return actor;
}

/*
 * Close the Actor's Lua state and free the memory the Actor is using.
 */
void
actor_destroy (Actor *actor)
{
    printf("Destroying Actor %d\n", actor->id);
    pthread_mutex_destroy(&actor->mutex);
    lua_close(actor->L);
    free(actor);
}
