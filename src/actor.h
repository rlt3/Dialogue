#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include "dialogue.h"

typedef struct Actor Actor;

Actor *
actor_create (lua_State *L);

void
actor_assign_id (void *actor, int id);

int
actor_get_id (void *actor);

void
actor_destroy (void *);


#endif
