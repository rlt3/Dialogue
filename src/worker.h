#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"

typedef struct Worker Worker;

/*
 * Create a Worker, which spawns a thread.
 * Returns NULL on failure.
 */
Worker *
worker_start ();

/*
 * Have the worker take the action on top of the given Lua stack. 
 * Pops the top of the given Lua stack if the action is taken.
 * Returns 1 if the action is taken, 0 if busy.
 */
int
worker_pop_action (Worker *worker, lua_State *L);

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 * Frees the worker and releases its reference.
 */
void
worker_stop (Worker *worker);

/*
 * Frees the worker.
 */
void
worker_cleanup (Worker *worker);

#endif
