#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"
#include "director.h"
#include "actor.h"

typedef struct Worker Worker;


Worker *
worker_start (lua_State *L, Director *director);

/*
 * Have the worker take the action on top of the given Lua stack. 
 * Pops the top of the given Lua stack if the action is taken.
 * Returns 1 if the action is taken, 0 if busy.
 */
int
worker_take_action (lua_State *L, Worker *worker);

/*
 * Save an Actor's pointer to a table in the Worker's Lua state so that it can
 * be easily cleaned up.
 */
void
worker_save_actor (lua_State *W, Actor *actor);

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 * Frees the worker and releases its reference.
 */
void
worker_stop (Worker *worker);

/*
 * Frees the worker. Frees any Actors it created.
 */
void
worker_cleanup (Worker *worker);

#endif
