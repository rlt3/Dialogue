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
 * If the return is not NULL, then it is successful.
 */
Actor *
actor_create (lua_State *L);

/*
 * Load any Scripts which are marked to be loaded (or reloaded). This function
 * will error out through the main Lua state L.
 */
void
actor_load (Actor *actor, lua_State *L);

void
actor_assign_id (void *actor, int id);

int
actor_get_id (void *actor);

void
actor_destroy (void *);


#endif
