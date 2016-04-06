#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "console.h"
#include "worker.h"
#include "company.h"
#include "utils.h"

struct Worker {
    lua_State *L; /* worker state */
    lua_State *M; /* mailbox stack */
    pthread_t thread;
    pthread_mutex_t mail_mutex;
    pthread_mutex_t state_mutex;
};

/*
 * Actions are just serialized object calls where the first element is the
 * actor (in integer, table, or string form) and the second element is the
 * method. Elements 3+ are arguments to that method.
 *
 * This function is used to actually *do* that method call. All errors should
 * occur through here as, ideally, all functionality of a program written for
 * Dialogue should be in the message handlers.
 *
 * Examples:
 * {a0, "load"} => a0:load()
 * {a0, "send", {"draw", "player.jpg"}} => a0:send{"draw", "player.jpg"}
 */
int
worker_catch (lua_State *W)
{
    int i, len;
    const int action_arg = 1;
    const char *message = NULL;
    /* positions within the Action table */
    const int actor_pos = 1;
    const int method_pos = 2;
    const int args_pos = 3;

    luaL_checktype(W, action_arg, LUA_TTABLE);
    len = luaL_len(W, action_arg);

    if (len < 2)
        luaL_error(W, 
                "Invalid length `%d' for Action given! Length of 2 required.",
                len);

    lua_rawgeti(W, action_arg, actor_pos);
    company_push_actor(W, company_actor_id(W, -1));

    lua_rawgeti(W, action_arg, method_pos);
    message = lua_tostring(W, -1);
    lua_gettable(W, -2);

    if (!lua_isfunction(W, -1))
        luaL_error(W, "Invalid method `%s' for Action given!", message);

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
        console_log("Action failed: %s\n", lua_tostring(W, -1));
        lua_pop(W, 1);
    }
}

/*
 * The Worker check its mailbox every loop. If no actions have been sent, it 
 * sleeps for a period and then checks again. It checks for a `nil' action as a
 * sentinel to quit.
 */
void *
worker_thread (void *arg)
{
    Worker *worker = arg;
    lua_State *W = worker->L;
    lua_State *M = worker->M;

    pthread_mutex_lock(&worker->state_mutex);

    while (1) {
        pthread_mutex_lock(&worker->mail_mutex);
        utils_transfer(W, M, lua_gettop(M));
        pthread_mutex_unlock(&worker->mail_mutex);

        if (lua_gettop(W) == 0) {
            usleep(1000);
            continue;
        }

        if (lua_isnil(W, -1))
            break;

        worker_process_action(W);
    }

    pthread_mutex_unlock(&worker->state_mutex);

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

    worker->M = luaL_newstate();

    if (!worker->M) {
        lua_close(worker->L);
        free(worker);
        worker = NULL;
        goto exit;
    }

    luaL_openlibs(worker->L);
    company_set(worker->L);
    pthread_mutex_init(&worker->mail_mutex, NULL);
    pthread_mutex_init(&worker->state_mutex, NULL);

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
 * With an Action on top of L, check to see if the Worker's mailbox is ready 
 * to accept Actions. If it is, the Action is popped off L into the mailbox
 * and return 0. If it isn't, the Action isn't popped and returns 1.
 */
int
worker_take_action (Worker *worker, lua_State *L)
{
    int rc = pthread_mutex_trylock(&worker->mail_mutex);
    if (rc == EBUSY || rc != 0)
        return 1;
    utils_transfer(worker->M, L, 1);
    pthread_mutex_unlock(&worker->mail_mutex);
    return 0;
}

/*
 * Block and wait for the Worker's mailbox. Pop the Action off L onto the 
 * Worker's mailbox state. Returns 0 if successful.
 */
int
worker_give_action (Worker *worker, lua_State *L)
{
    if (pthread_mutex_lock(&worker->mail_mutex) != 0)
        return 1;
    utils_transfer(worker->M, L, 1);
    pthread_mutex_unlock(&worker->mail_mutex);
    return 0;
}

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 */
void
worker_stop (Worker *worker)
{
    pthread_mutex_lock(&worker->mail_mutex);
    lua_pushnil(worker->M); /* it checks for nil as a sentinel to stop */
    pthread_mutex_unlock(&worker->mail_mutex);
    pthread_join(worker->thread, NULL);
}

/*
 * Frees the worker.
 */
void
worker_cleanup (Worker *worker)
{
    /* 
     * Wait for the state first because it will only become unlocked after the
     * thread is done. this makes it safe for us to then destroy the mailbox
     * state afterwards.
     */
    pthread_mutex_lock(&worker->state_mutex);
    lua_close(worker->L);
    pthread_mutex_unlock(&worker->state_mutex);

    pthread_mutex_lock(&worker->mail_mutex);
    lua_close(worker->M);
    pthread_mutex_unlock(&worker->mail_mutex);

    free(worker);
}
