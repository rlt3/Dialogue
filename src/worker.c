#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "worker.h"
#include "company.h"
#include "utils.h"

struct Worker {
    lua_State *L;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

/*
 * Why not make Actions just a serialized object method call for a specific
 * actor? This means that potentially all methods of an actor can be 
 * 'asynchronous' if you just throw them into the Director.
 *
 * {a0, "load"} => a0:load()
 * {a0, "send", "draw", "player.jpg"} => a0:send("draw", "player.jpg")
 */
int
worker_catch (lua_State *W)
{
    int i, len;
    const int action_arg = 1;
    const char *message = NULL;
    /* positions within the table */
    const int actor_pos = 1;
    const int method_pos = 2;
    const int args_pos = 3;

    luaL_checktype(W, action_arg, LUA_TTABLE);
    len = luaL_len(W, action_arg);

    if (len < 2)
        luaL_error(W, "Invalid length for an Action: %d", len);

    lua_rawgeti(W, action_arg, actor_pos);
    company_push_actor(W, company_actor_id(W, -1));

    lua_rawgeti(W, action_arg, method_pos);
    message = lua_tostring(W, -1);
    lua_gettable(W, -2);

    if (!lua_isfunction(W, -1))
        luaL_error(W, "Invalid method for an Action: %s", message);

    /* push `self' reference */
    lua_pushvalue(W, -2);

    /* push rest of args */
    for (i = args_pos; i <= len; i++)
        lua_rawgeti(W, action_arg, i);

    /* 
     * push length of table minus 2 (skip first two elements) and plus one for
     * the `self`
     */
    lua_call(W, len - 1, 0);

    return 0;
}

void
worker_process_action (lua_State *W)
{
    lua_pushcfunction(W, worker_catch);
    lua_insert(W, -2);
    if (lua_pcall(W, 1, 0, 0)) {
        printf("Action failed: %s\n", lua_tostring(W, -1));
        lua_pop(W, 1);
    }
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
    company_set(worker->L);
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
