#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "worker.h"
#include "mailbox.h"

struct Worker {
    lua_State *L;
    pthread_t thread;
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
    Worker *worker = arg;
    lua_State *W = worker->L;
    const int dialogue_table = 1;
    int i, top_action;

    /* push the initial two tables onto the stack */
    lua_getglobal(W, "Dialogue");

    while (worker->working) {
        if (mailbox_pop_all(W, worker->mailbox) == 0)
            continue;
        
        top_action = lua_gettop(W);

        for (i = top_action; i > dialogue_table; i--) {
            worker->processed++;
            lua_pop(W, 1);
        }
    }

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
    worker->working = 0;
    pthread_join(worker->thread, NULL);
    printf("%p processed %d\n", worker, worker->processed);
    mailbox_destroy(L, worker->mailbox);
    free(worker);
}
