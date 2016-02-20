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
 * Pop the error from the Actor onto the given Lua stack.
 */
void
actor_pop_error (Actor *actor, lua_State *L);

/*
 * Load any Scripts which are marked to be loaded (or reloaded). Errors from
 * Scripts are caught in sequential order. Meaning an error for the first
 * Script will mask errors for any remaining. Errors are left on top of the
 * Actor's stack and 1 is returned. Otherwise, success, and returns 0.
 */
int
actor_load (Actor *actor);

int
actor_probe (Actor *actor, lua_State *L);

/*
 * The Actor sends the message to all of its Scripts which are loaded. Errors
 * from Scripts are caught in sequential order. Meaning an error for the first
 * Script will mask errors for any remaining. Errors are left on top of the
 * Actor's stack and 1 is returned. Otherwise, success, and returns 0.
 */
int
actor_send (Actor *actor, lua_State *L);

void
actor_assign_id (void *actor, int id);

int
actor_get_id (void *actor);

void
actor_destroy (void *);


#endif
