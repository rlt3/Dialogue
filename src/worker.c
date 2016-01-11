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
    const char *error;
    const int dialogue_table = 1;
    int i, len, args, top;
    Worker *worker = arg;
    lua_State *W = worker->L;

    /* push the initial two tables onto the stack */
    //lua_getglobal(W, "Dialogue");

get_work:
    while (worker->working) {
        if (mailbox_pop_all(W, worker->mailbox) == 0)
            goto get_work;
        
        for (top = lua_gettop(W); top > dialogue_table; top--) {
        //    len  = luaL_len(W, top);
        //    args = len - 1;

        //    /* push the action type to see if it exists */
        //    lua_rawgeti(W, top, 1);
        //    lua_gettable(W, dialogue_table);

        //    if (!lua_isfunction(W, -1)) {
        //        lua_rawgeti(W, top, 1);
        //        error = lua_tostring(W, -1);
        //        printf("`%s' is not an Action recognized by Dialogue!\n", error);
        //        lua_pop(W, 2); /* action type and the `function' */
        //        goto next;
        //    }

        //    /* push the rest of the table as arguments */
        //    for (i = 2; i <= len; i++)
        //        lua_rawgeti(W, top, i);

        //    if (lua_pcall(W, args, 0, 0)) {
        //        error = lua_tostring(W, -1);
        //        printf("%s\n", error);
        //        lua_pop(W, 1); /* error string */
        //    }
next:
            worker->processed++;
            lua_pop(W, 1); /* the top action */
        }
    }

    return NULL;
}

Worker *
worker_start (lua_State *L)
{
    Worker *worker = malloc(sizeof(*worker));
    /* TODO: check memory here */
    worker->L = luaL_newstate();
    /* TODO: check memory here */
    worker->mailbox = mailbox_create();
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
    mailbox_destroy(worker->mailbox);
    free(worker);
}