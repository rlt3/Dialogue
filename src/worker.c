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
    Mailbox *mailbox;
    int processed;
    int working;
};

void
worker_process_action (Worker *worker, const int action_table)
{
    lua_State *W = worker->L;
    //int len  = luaL_len(W, action_table);
    //int args = len - 1;
    //int i;

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
    }

    lua_pop(W, 1);
}

void *
worker_thread (void *arg)
{
    Worker *worker = arg;
    lua_State *W = worker->L;
    int top, working;

    while (1) {
        pthread_mutex_lock(&worker->mutex);
        working = worker->working;
        pthread_mutex_unlock(&worker->mutex);

        if (!working)
            break;

	/* 
         * TODO: should be a cond_wait on if the mailbox is empty or not.
         */
        if (mailbox_pop_all(W, worker->mailbox) == 0) {
            usleep(1000);
            continue;
        }

        for (top = lua_gettop(W); top > 1; top--)
            worker_process_action(worker, top);
    }

    return NULL;
}

///*
// * Actually do the actions of our Dialogue.
// */
//void *
//worker_thread (void *arg)
//{
//    const char *error;
//    const int director_table = 1;
//    int i, len, args, top;
//    Worker *worker = arg;
//    lua_State *W = worker->L;
//
//    lua_getglobal(W, "Director");
//
//get_work:
//    while (worker->working) {
//        if (mailbox_pop_all(W, worker->mailbox) == 0)
//            goto get_work;
//        
//        for (top = lua_gettop(W); top > director_table; top--) {
//            len  = luaL_len(W, top);
//            args = len - 1;
//
//            /* push the action type to see if it exists */
//            lua_rawgeti(W, top, 1);
//            lua_gettable(W, director_table);
//
//            if (!lua_isfunction(W, -1)) {
//                lua_rawgeti(W, top, 1);
//                error = lua_tostring(W, -1);
//                printf("`%s' is not an Action recognized by Dialogue!\n", error);
//                lua_pop(W, 2); /* action type and the `function' */
//                goto next;
//            }
//
//            /* push the director's 'self' reference for the method call */
//            lua_pushvalue(W, director_table);
//
//            /* push the rest of the table as arguments */
//            for (i = 2; i <= len; i++)
//                lua_rawgeti(W, top, i);
//
//            /* push the Worker's pointer at the end as an optional argument */
//            lua_pushlightuserdata(W, worker);
//
//            if (lua_pcall(W, args + 2, 0, 0)) {
//                error = lua_tostring(W, -1);
//                printf("%s\n", error);
//                lua_pop(W, 1); /* error string */
//            }
//
//            printf("top: %d\n", lua_gettop(W));
//next:
//            worker->processed++;
//            lua_pop(W, 1); /* the top action */
//        }
//    }
//
//    return NULL;
//}

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

    worker->working = 1;
    worker->processed = 0;
    luaL_openlibs(worker->L);
    pthread_mutex_init(&worker->mutex, NULL);

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
    return mailbox_push_top(L, worker->mailbox);
}

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 */
void
worker_stop (Worker *worker)
{
    pthread_mutex_lock(&worker->mutex);
    worker->working = 0;
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
