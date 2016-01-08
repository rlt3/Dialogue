#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "worker.h"
#include "utils.h"

struct Worker {
    lua_State *L;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t wait_cond;
    int working;
    int waiting;
    int ref;
};

/*
 * Actually do the actions of our Dialogue.
 *
 * _Always_ has the Dialogue table at index 1. At every loop it expects the 
 * action table at index 2. Pushes the first element of the action table to see
 * if it exists in the Dialogue table.
 *
 * If the action exists, the action function is called and the rest of the elements
 * in the action table are used as arguments, if any. If there is an error with
 * the function call an error is printed.
 *
 * If the action doesn't exist it prints an error.
 *
 * The worker then cleans up the stack for the next loop and then waits for an
 * action table to be placed on its stack.
 */
void *
worker_thread (void *arg)
{
    const char *error;
    const int dialogue_table = 1;
    const int action_table = 2;
    Worker *worker = arg;
    lua_State *W = worker->L;
    int i, args, len;

    pthread_mutex_lock(&worker->mutex);
    lua_getglobal(W, "Dialogue");

    while (worker->working) {
        if (lua_gettop(W) != action_table)
            goto wait;

        /* -1 because the first element is always an action as a string */
        len  = luaL_len(W, action_table);
        args = len - 1;

        lua_rawgeti(W, action_table, 1);
        lua_gettable(W, dialogue_table);

        if (!lua_isfunction(W, -1)) {
            lua_rawgeti(W, action_table, 1);
            error = lua_tostring(W, -1);
            printf("`%s' is not an Action recognized by Dialogue1\n", error);
            lua_pop(W, 2); /* action and the not function */
            goto wait;
        }

        for (i = 2; i <= len; i++)
            lua_rawgeti(W, action_table, i);

        /*
         * Right here, we can return a boolean to see if we resend the action
         * or not.
         * 
         * This is also a special place -- I have the direct worker available
         * for a return value which can be more actions to send! If I handled
         * tones here, I could have the specific actor's actions inserted here
         * before it needs to wait.
         *
         * this also could be an option for messages that may want to be sent
         * sequentially?
         */

        /*
         * Returns a table of actions to resend and a boolean to determine if
         * the messages should be resent through the Director or if this Worker
         * should just redo it
         */
        if (lua_pcall(W, args, 0, 0)) {
            error = lua_tostring(W, -1);
            printf("%s\n", error);
            lua_pop(W, 1); /* error string */
        }

       lua_pop(W, 1); /* action table */

wait:
        worker->waiting = 1;
        while (worker->waiting)
            pthread_cond_wait(&worker->wait_cond, &worker->mutex);
    }

    pthread_mutex_unlock(&worker->mutex);

    return NULL;
}

Worker *
worker_start (lua_State *L)
{
    /* TODO: check memory here */
    Worker *worker = malloc(sizeof(*worker));
    worker->L = lua_newthread(L);
    worker->working = 1;
    worker->waiting = 0;
    /* ref (which pops) the thread object so we control garbage collection */
    worker->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    pthread_mutex_init(&worker->mutex, NULL);
    pthread_cond_init(&worker->wait_cond, NULL);
    pthread_create(&worker->thread, NULL, worker_thread, worker);

    return worker;
}

/*
 * Have the worker take the action on top of the given Lua stack. 
 * Pops the top of the given Lua stack if the action is taken.
 * Returns 1 if the action is taken, 0 if busy.
 */
int
worker_take_action (lua_State *L, Worker *worker)
{
    int rc = pthread_mutex_trylock(&worker->mutex);

    if (rc == EBUSY)
        return 0;

    if (!worker->waiting) {
        pthread_mutex_unlock(&worker->mutex);
        return 0;
    }

    /* TODO: in the real system this must be utils_move_top. */
    lua_xmove(L, worker->L, 1);
    worker->waiting = 0;
    pthread_mutex_unlock(&worker->mutex);
    pthread_cond_signal(&worker->wait_cond);

    return 1;
}

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 * Frees the worker and releases its reference.
 */
void
worker_stop (lua_State *L, Worker *worker)
{
    pthread_mutex_lock(&worker->mutex);
    worker->working = 0;
    worker->waiting = 0;
    pthread_mutex_unlock(&worker->mutex);
    pthread_cond_signal(&worker->wait_cond);

    pthread_join(worker->thread, NULL);
    luaL_unref(L, LUA_REGISTRYINDEX, worker->ref);
    free(worker);
}
