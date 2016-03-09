#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"

typedef struct Worker Worker;

/*
 * Create a worker without a thread. Assigns the thread to main if it isn't 
 * null. Returns NULL on failure.
 */
Worker *
worker_create (lua_State *main);

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
 * A non-blocking function which checks the state of the Worker. If the Worker
 * needs work, the action on top of L is popped onto the Worker's state and 0
 * is returned. If the Worker is busy, 0 is returned.
 */
int
worker_take_action (Worker *worker, lua_State *L);

/*
 * Block and wait for the Worker to become free and pop the Action off L onto
 * the Worker's state. Returns 0 if successful.
 */
int
worker_give_action (Worker *worker, lua_State *L);

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
