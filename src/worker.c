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
            lua_pop(W, 1);
            error = "Bad Action!";
            printf("%s\n", error);
            goto wait;
        }

        for (i = 2; i <= len; i++)
            lua_rawgeti(W, action_table, i);

        if (lua_pcall(W, args, 0, 0)) {
            error = lua_tostring(W, -1);
            printf("%s\n", error);
            lua_pop(W, 1);
            goto wait;
        }

        lua_pop(W, 1);

wait:
        worker->waiting = 1;
        while (worker->waiting)
            pthread_cond_wait(&worker->wait_cond, &worker->mutex);

//error:  /* incidentally loops around and ends up at `wait' */
//        lua_getfield(W, dialogue_table, "error");
//        lua_pushstring(W, error);
//        lua_call(W, 1, 0);
//        lua_pop(W, 1);
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
    /* worker->L = luaL_newstate(); */
    worker->working = 1;
    worker->waiting = 0;
    worker->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    pthread_mutex_init(&worker->mutex, NULL);
    pthread_cond_init(&worker->wait_cond, NULL);
    pthread_create(&worker->thread, NULL, worker_thread, worker);

    return worker;
}

/*
 * Have the worker take the action on top of the given Lua stack. 
 * Returns 1 if the action was taken, 0 if busy.
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

void
worker_stop (lua_State *L, Worker *worker)
{
    pthread_mutex_lock(&worker->mutex);
    worker->working = 0;
    worker->waiting = 0;
    pthread_mutex_unlock(&worker->mutex);
    pthread_cond_signal(&worker->wait_cond);

    luaL_unref(L, LUA_REGISTRYINDEX, worker->ref);
    pthread_join(worker->thread, NULL);
    free(worker);
}
