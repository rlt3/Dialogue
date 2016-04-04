#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"

typedef struct Worker Worker;

/*
 * Create a worker without a thread. Assigns the thread to main if it isn't 
 * null. Returns NULL on failure.
 */
Worker *
worker_create ();

/*
 * Create a Worker and start it, which spawns a thread.
 * Returns NULL on failure.
 */
Worker *
worker_start ();

void*
worker_thread (void *arg);

/* 
 * Process the next Action.
 */
void
worker_process_action (lua_State *W);

/*
 * With an Action on top of L, check to see if the Worker's mailbox is ready 
 * to accept Actions. If it is, the Action is popped off L into the mailbox
 * and return 0. If it isn't, the Action isn't popped and returns 1.
 */
int
worker_take_action (Worker *worker, lua_State *L);

/*
 * Block and wait for the Worker's mailbox. Pop the Action off L onto the 
 * Worker's mailbox state. Returns 0 if successful.
 */
int
worker_give_action (Worker *worker, lua_State *L);

/*
 * Block and wait for the Worker to be free. Get the Worker's Lua state. This
 * causes the Worker to wait until `worker_return_state` has been called. 
 * Returns NULL if an error occurred. `worker_return_state` doesn't need to be
 * called if this returns NULL.
 */
lua_State *
worker_request_state (Worker *worker);

/*
 * Return the state to the Worker so it an resume working. Produces undefined
 * behavior is this function is called when `worker_request_state` returns NULL.
 */
void
worker_return_state (Worker *worker);

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 */
void
worker_stop (Worker *worker);

/*
 * Frees the worker.
 */
void
worker_cleanup (Worker *worker);

#endif
