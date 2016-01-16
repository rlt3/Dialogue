#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include "dialogue.h"
#include "director.h"

typedef struct Actor Actor;

Actor *
actor_create (lua_State *L, int id);

/*
 * Close the Actor's Lua state and free the memory the Actor is using.
 */
void
actor_destroy (Actor *actor);


#endif
