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
    pthread_cond_t wait_cond;
    Mailbox *mailbox;
    int processed;
    int working;
    int ref;
};

/*
 * Actually do the actions of our Dialogue.
 */
void *
worker_thread (void *arg)
{
    const char *error;
    const int dialogue_table = 1;
    int i, len, args, top;
    Worker *worker = arg;
    lua_State *W = worker->L;

    pthread_mutex_lock(&worker->mutex);

    /* push the initial two tables onto the stack */
    lua_getglobal(W, "Dialogue");

    while (1) {
        if (!worker->working)
            break;

        /* wait if there's no work */
        if (mailbox_pop_all(W, worker->mailbox) == 0) {
            printf("%p waiting %p\n", worker, worker->mailbox);
            pthread_cond_wait(&worker->wait_cond, &worker->mutex);
            printf("%p signaled\n", worker);
            continue;
        }
        
        printf("%p working\n", worker);
        for (top = lua_gettop(W); top > dialogue_table; top--) {
            worker->processed++;
            lua_pop(W, 1); /* the top action */
        }
    }

    if (top > 1)
        printf("%p quit with %d left\n", worker, top);

    pthread_mutex_unlock(&worker->mutex);

    return NULL;
}

void
worker_wake (Worker *worker)
{
    pthread_cond_signal(&worker->wait_cond);
}

Worker *
worker_start (lua_State *L)
{
    /* TODO: check memory here */
    Worker *worker = malloc(sizeof(*worker));
    worker->L = lua_newthread(L);
    /* ref (which pops) the thread object so we control garbage collection */
    worker->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    worker->mailbox = mailbox_create(L, worker);
    worker->working = 1;
    worker->processed = 0;
    pthread_mutex_init(&worker->mutex, NULL);
    pthread_cond_init(&worker->wait_cond, NULL);
    pthread_create(&worker->thread, NULL, worker_thread, worker);

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
    /* wait for the worker to be waiting for work */
    pthread_mutex_lock(&worker->mutex);
    worker->working = 0;
    pthread_mutex_unlock(&worker->mutex);
    pthread_cond_signal(&worker->wait_cond);

    pthread_join(worker->thread, NULL);
    printf("%p processed %d\n", worker, worker->processed);
    mailbox_destroy(L, worker->mailbox);
    free(worker);
}
