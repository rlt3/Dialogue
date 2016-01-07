#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"

typedef struct Worker Worker;


Worker *
worker_start (lua_State *L);

/*
 * Have the worker take the action on top of the given Lua stack. 
 * Pops the top of the given Lua stack if the action is taken.
 * Returns 1 if the action is taken, 0 if busy.
 */
int
worker_take_action (lua_State *L, Worker *worker);

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 * Frees the worker and releases its reference.
 */
void
worker_stop (lua_State *L, Worker *worker);

#endif
