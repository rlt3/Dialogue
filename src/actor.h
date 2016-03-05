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
 * The Actor loads (or reloads) all of its Scripts marked to be loaded. 
 *
 * An optional argument specifying which Scripts should be forcefully loaded.
 * If one passes the string "all" then all Scripts will be loaded. If one
 * passes an integer, that is the nth Script which will be loaded.
 *
 * Errors from Scripts are caught in sequential order. Meaning an error for the
 * first Script will mask errors for any remaining. Errors are left on top of
 * the Actor's stack and 1 is returned. Otherwise, success, and returns 0.
 */
int
actor_load (Actor *actor, lua_State *L);

/*
 * Expects two items on top of L: an integer which is the nth Script of the 
 * Actor and the field of that Script to probe.
 *
 * If the Script could not be found, it returns 1 and leaves an error message
 * on top of the Actor's stack.
 */
int
actor_probe (Actor *actor, lua_State *L);

/*
 * The Actor sends the message to all of its Scripts which are loaded.
 *
 * Assumes a message table on top of the given Lua stack in the form of:
 *      { 'message' [, arg1 [, ... [, argn]]], author}
 *
 * The function copies the table from the given Lua stack (L) onto the Actor's
 * Lua stack for all Scripts to access.
 *
 * Errors from Scripts are caught in sequential order. Meaning an error for the
 * first Script will mask errors for any remaining. Errors are left on top of
 * the Actor's stack and 1 is returned. Otherwise, success, and returns 0.
 *
 * A special Error will occur when an Actor is asked to handle a message with
 * no loaded Scripts.
 */
int
actor_send (Actor *actor, lua_State *L);

void
actor_assign_id (void *actor, int id);

void
actor_destroy (void *);

#endif
