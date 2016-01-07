#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"

typedef struct Worker Worker;


Worker *
worker_start (lua_State *L);

/*
 * Have the worker take the action on top of the given Lua stack. 
 * Returns 1 if the action was taken, 0 if busy.
 */
int
worker_take_action (lua_State *L, Worker *worker);

void
worker_stop (lua_State *L, Worker *worker);

#endif
