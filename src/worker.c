#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "worker.h"
#include "mailbox.h"
#include "action.h"

struct Worker {
    lua_State *L;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    Mailbox *mailbox;
    enum { WORKING, WAITING, DONE } state;
    int processed;
};

/*
 * All Director Actions happen one at a time -- we don't need the ability to 
 * have a mailbox and the mailbox is actually redudant. Worker should process
 * a single action at a time and then wait for input (another action).
 */

/* worker takes action from L if mutex is unlocked, i.e. state == WAITING */
//int
//worker_take_action (Worker *worker, lua_State *L);

/* 
 * looks at the top of W (the worker's stack) and processes the action. this is
 * a `catch' function around the Lua actor object methods (implemented in
 * Company.h). Basically this function *is* the pcall function for the entire
 * system and is where errors are printed.
 */
//int
//worker_process_action (lua_State *W);

void
worker_process_action (Worker *worker, const int action_table)
{
    lua_State *W = worker->L;
    //int len  = luaL_len(W, action_table);
    //int args = len - 1;
    //int i;
    
    printf("%p got action @ %d!\n", (void*) worker, action_table);

    lua_rawgeti(W, action_table, 1);

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

void *
worker_thread (void *arg)
{
    Worker *worker = arg;
    lua_State *W = worker->L;
    int top;

    /*
    enum { WORKING, WAITING, DONE } state;
    while (1) {
        pthread_mutex_lock(&worker->mutex);
        state = worker->state;
        pthread_mutex_unlock(&worker->mutex);

        if (state == DONE)
            break;

        if (mailbox_pop_all(W, worker->mailbox) == 0) {
            usleep(1000);
            continue;
        }

        for (top = lua_gettop(W); top > 0; top--)
            worker_process_action(worker, top);
    }
    */

    pthread_mutex_lock(&worker->mutex);
    printf("Spinning up %p\n", (void*) worker);

    while (worker->state != DONE) {
        /* 
         * TODO: Need to be able to wake from Mailbox when worker "goes to
         * sleep" with a minimal number of locks.
         *
         * Perhaps N retrys before acquiring the mailbox's lock, setting
         * a flag which tells the mailbox to signal the worker on the
         * next action it receives.
         */
        if (mailbox_pop_all(W, worker->mailbox) == 0)
            pthread_cond_wait(&worker->cond, &worker->mutex);

        for (top = lua_gettop(W); top > 0; top--)
            worker_process_action(worker, top);
    }

    printf("Spinning down %p\n", (void*) worker);
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

    worker->mailbox = mailbox_create();

    if (!worker->mailbox) {
        lua_close(worker->L);
        free(worker);
        worker = NULL;
        goto exit;
    }

    worker->state = WORKING;
    worker->processed = 0;

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
 * Pop the Action from the Lua stack onto the Worker's mailbox.
 * Returns 1 if the action is taken, 0 if busy.
 */
int
worker_pop_action (Worker *worker, lua_State *L)
{
    int ret = mailbox_push_top(L, worker->mailbox);

    /*
    if (ret)
        pthread_cond_signal(&worker->cond);
    */

    return ret;
}

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 */
void
worker_stop (Worker *worker)
{
    pthread_mutex_lock(&worker->mutex);
    worker->state = DONE;
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
    mailbox_destroy(worker->mailbox);
    lua_close(worker->L);
    free(worker);
}
