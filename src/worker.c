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
    const int action_index = 1;
    Worker *worker = arg;
    lua_State *W = worker->L;
    int i, len;

    pthread_mutex_lock(&worker->mutex);

    while (worker->working) {
        if (lua_gettop(W) != action_index)
            goto wait;

        len = luaL_len(W, action_index);
        printf("%p {", worker);
        for (i = 1; i <= len; i++) {
            lua_rawgeti(W, action_index, i);
            printf(" %s ", lua_tostring(W, -1));
            lua_pop(W, 1);
        }
        printf("}\n");
        lua_pop(W, 1);

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
