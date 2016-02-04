#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include "dialogue.h"

typedef struct Actor Actor;

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
 * When the thread_id given is greater than -1, the system will make sure the
 * Actor is always ran on the thread that represents the id. 0 always represents
 * the main thread.
 *
 * For syntax and formatting errors, this will call luaL_error. For system-level
 * errors, this returns NULL (and cleans up any memory) in every case. This 
 * function should be wrapped and an appropriate luaL_error should be called.
 *
 * If the return is not NULL, then it is successful.
 */
Actor *
actor_create (lua_State *L, int thread_id);

void
actor_assign_id (void *actor, int id);

int
actor_get_id (void *actor);

void
actor_destroy (void *);


#endif
