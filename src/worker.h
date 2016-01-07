#ifndef DIALOGUE_WORKER
#define DIALOGUE_WORKER

#include "dialogue.h"

typedef struct Worker Worker;

/*
 * Have the worker take the action on top of the given Lua stack. Returns 1 if
 * the action was taken, 0 if busy.
 */
int
lua_worker_take_top (lua_State *L, Worker *worker);

#endif
