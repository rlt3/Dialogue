#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"

typedef struct Worker Worker;

/*
 * Create a worker without a thread.
 * Returns NULL on failure.
 */
Worker *
worker_create ();

/*
 * Create a Worker and start it, which spawns a thread.
 * Returns NULL on failure.
 */
Worker *
worker_start ();

/* 
 * Process the next Action.
 */
void
worker_process_action (lua_State *W);

/*
 * Worker takes action off the top of L if the worker needs an action. Returns
 * 0 if the worker took the action and 1 if not.
 */
int
worker_take_action (Worker *worker, lua_State *L);

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
