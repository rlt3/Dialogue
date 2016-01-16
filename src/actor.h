#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include "dialogue.h"
#include "director.h"
#include <pthread.h>

typedef struct Actor {
    lua_State *L;
    pthread_mutex_t mutex;
    int is_lead; /* restrict to a single thread */
    int is_star; /* restrict to the main thread */
    int id;      /* id inside the Company */
} Actor;

Actor *
actor_create (lua_State *L, int id);

/*
 * Close the Actor's Lua state and free the memory the Actor is using.
 */
void
actor_destroy (Actor *actor);


#endif
