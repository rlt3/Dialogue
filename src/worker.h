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
 * Process the next Action from the Worker's mailbox.
 */
void
worker_process_action (Worker *worker, const int action_table);

/*
 * Pop the Action from the Lua stack onto the Worker's mailbox.
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
