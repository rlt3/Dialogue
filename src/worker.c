#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "worker.h"
#include "action.h"
#include "utils.h"

struct Worker {
    lua_State *L;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

void
worker_process_action (lua_State *W)
{
    int top = lua_gettop(W);

    lua_rawgeti(W, top, 1);

    switch (lua_tointeger(W, -1)) {
    case ACTION_SEND:
        puts("Send");
        break;

    case ACTION_DELIVER:
        puts("Deliver");
        break;
        
    case ACTION_LOAD:
        puts("Load");
        break;

    case ACTION_BENCH:
        puts("Bench");
        break;

    case ACTION_JOIN:
        puts("Join");
        break;

    case ACTION_DELETE:
        puts("Delete");
        break;

    default:
        puts("No Action!!!");
        break;
    }

    lua_pop(W, 2); /* first element of table & table itself */
}

/*
 * The Worker thread was designed to handle a single message at a time but can
 * handle more if somehow the Worker is faced with >1 Action on its stack.
 *
 * The Worker will loop forever until it finds `nil` as an Action, which is how
 * `worker_stop` is implemented.
 */
void *
worker_thread (void *arg)
{
    Worker *worker = arg;
    lua_State *W = worker->L;

    pthread_mutex_lock(&worker->mutex);

    while (1) {
        while (lua_gettop(W) == 0)
            pthread_cond_wait(&worker->cond, &worker->mutex);

        if (lua_isnil(W, -1))
            break;

        worker_process_action(W);
    }

    pthread_mutex_unlock(&worker->mutex);

    return NULL;
}

/*
 * Create a worker without a thread.
 * Returns NULL on failure.
 */
Worker *
worker_create ()
{
    Worker *worker = malloc(sizeof(*worker));
    
    if (!worker)
        goto exit;

    worker->L = luaL_newstate();

    if (!worker->L) {
        free(worker);
        worker = NULL;
        goto exit;
    }

    luaL_openlibs(worker->L);
    pthread_mutex_init(&worker->mutex, NULL);
    pthread_cond_init(&worker->cond, NULL);

exit:
    return worker;
}

/*
 * Create a Worker and start it, spawning a thread.
 * Returns NULL on failure.
 */
Worker *
worker_start ()
{
    Worker *worker = worker_create();
    
    if (!worker)
        goto exit;

    pthread_create(&worker->thread, NULL, worker_thread, worker);

exit:
    return worker;
}

/*
 * Worker takes action off the top of L if the worker needs an action. Returns
 * 0 if the worker took the action and 1 if not.
 */
int
worker_take_action (Worker *worker, lua_State *L)
{
    int rc = pthread_mutex_trylock(&worker->mutex);

    if (rc == EBUSY)
        return 1;

    utils_transfer(worker->L, L, 1);
    pthread_cond_signal(&worker->cond);
    pthread_mutex_unlock(&worker->mutex);
    return 0;
}

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 */
void
worker_stop (Worker *worker)
{
    pthread_mutex_lock(&worker->mutex);
    lua_pushnil(worker->L); /* it checks for nil as a sentinel to stop */
    pthread_cond_signal(&worker->cond);
    pthread_mutex_unlock(&worker->mutex);

    pthread_join(worker->thread, NULL);
}

/*
 * Frees the worker.
 */
void
worker_cleanup (Worker *worker)
{
    lua_close(worker->L);
    free(worker);
}
