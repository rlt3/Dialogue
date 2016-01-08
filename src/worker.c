#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "worker.h"
#include "mailbox.h"

struct Worker {
    lua_State *L;
    pthread_t thread;
    pthread_mutex_t mutex;
    Mailbox *mailbox;
    int processed;
    int working;
    int ref;
};

/*
 * Actually do the actions of our Dialogue.
 *
 * _Always_ has the Dialogue table at index 1. At every loop it expects an
 * action table on top. Pushes the first element of the action table to see
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
    Worker *worker = arg;
    lua_State *W = worker->L;
    int len;

    /* push the initial two tables onto the stack */
    lua_getglobal(W, "Dialogue");

    while (worker->working) {
        pthread_mutex_lock(&worker->mutex);

        mailbox_pop_all(W, worker->mailbox);
        len = luaL_len(W, -1);
        if (len > 0) {
            worker->processed += len;
            lua_pop(W, 1);
        }

        pthread_mutex_unlock(&worker->mutex);
    }

    printf("%p quit\n", worker);

    return NULL;
}

Worker *
worker_start (lua_State *L)
{
    /* TODO: check memory here */
    Worker *worker = malloc(sizeof(*worker));
    worker->L = lua_newthread(L);
    /* ref (which pops) the thread object so we control garbage collection */
    worker->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    worker->mailbox = mailbox_create(L);
    worker->working = 1;
    worker->processed = 0;
    pthread_mutex_init(&worker->mutex, NULL);
    //pthread_create(&worker->thread, NULL, worker_thread, worker);

    return worker;
}

/*
 * Call `mailbox_push_top' for the Worker's mailbox.
 */
int
worker_take_action (lua_State *L, Worker *worker)
{
    return mailbox_push_top(L, worker->mailbox);
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
    pthread_mutex_unlock(&worker->mutex);
    printf("%p stop\n", worker);
    //pthread_join(worker->thread, NULL);
    mailbox_destroy(L, worker->mailbox);
    printf("%p processed %d\n", worker, worker->processed);
    luaL_unref(L, LUA_REGISTRYINDEX, worker->ref);
    free(worker);
}
