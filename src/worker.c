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
    const char *error;
    const int dialogue_table = 1;
    Worker *worker = arg;
    lua_State *W = worker->L;
    int i, args, len, top_action;

    lua_getglobal(W, "Dialogue");

get_work:
    while (worker->working) {
        mailbox_pop_all(W, worker->mailbox);
        top_action = lua_gettop(W);

        if (top_action == dialogue_table)
            goto get_work;

        /* -1 because the first element is always an action as a string */
        len  = luaL_len(W, top_action);
        args = len - 1;

        lua_rawgeti(W, top_action, 1);
        lua_gettable(W, dialogue_table);

        if (!lua_isfunction(W, -1)) {
            lua_rawgeti(W, top_action, 1);
            error = lua_tostring(W, -1);
            printf("`%s' is not an Action recognized by Dialogue!\n", error);
            lua_pop(W, 2); /* action and the not function */
            goto next;
        }

        for (i = 2; i <= len; i++)
            lua_rawgeti(W, top_action, i);

        /*
         * Returns a table of actions to resend and a boolean to determine if
         * the messages should be resent through the Director or if this Worker
         * should just redo it
         *
         * 0 - Normal case, can pus nil for table
         * 1 - Resend the current message
         * 2 - Resend the list of messages
         * 3 - Redo - push the list onto the Worker stack and loop
         */
        if (lua_pcall(W, args, 0, 0)) {
            error = lua_tostring(W, -1);
            printf("%s\n", error);
            lua_pop(W, 1); /* error string */
        }

next:
       lua_pop(W, 1); /* action table */
    }

    return NULL;
}

Worker *
worker_start (lua_State *L)
{
    /* TODO: check memory here */
    Worker *worker = malloc(sizeof(*worker));
    worker->L = lua_newthread(L);
    worker->working = 1;
    worker->mailbox = mailbox_create(L);
    /* ref (which pops) the thread object so we control garbage collection */
    worker->ref = luaL_ref(L, LUA_REGISTRYINDEX);
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
    mailbox_destroy(L, worker->mailbox);
    luaL_unref(L, LUA_REGISTRYINDEX, worker->ref);
    free(worker);
}
