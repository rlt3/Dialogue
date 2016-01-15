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
    const int director_table = 1;
    int i, len, args, top;
    Worker *worker = arg;
    lua_State *W = worker->L;

    lua_getglobal(W, "Director");

get_work:
    while (worker->working) {
        if (mailbox_pop_all(W, worker->mailbox) == 0)
            goto get_work;
        
        for (top = lua_gettop(W); top > director_table; top--) {
            len  = luaL_len(W, top);
            args = len - 1;

            /* push the action type to see if it exists */
            lua_rawgeti(W, top, 1);
            lua_gettable(W, director_table);

            if (!lua_isfunction(W, -1)) {
                lua_rawgeti(W, top, 1);
                error = lua_tostring(W, -1);
                printf("`%s' is not an Action recognized by Dialogue!\n", error);
                lua_pop(W, 2); /* action type and the `function' */
                goto next;
            }

            /* push the director's 'self' reference for the method call */
            lua_pushvalue(W, director_table);

            /* push the rest of the table as arguments */
            for (i = 2; i <= len; i++)
                lua_rawgeti(W, top, i);

            /* push the Worker's pointer at the end as an optional argument */
            lua_pushlightuserdata(W, worker);

            if (lua_pcall(W, args + 2, 0, 0)) {
                error = lua_tostring(W, -1);
                printf("%s\n", error);
                lua_pop(W, 1); /* error string */
            }
next:
            worker->processed++;
            lua_pop(W, 1); /* the top action */
        }
    }

    return NULL;
}

Worker *
worker_start (lua_State *L, Director *director)
{
    Worker *worker = malloc(sizeof(*worker));
    /* TODO: check memory here */
    worker->L = luaL_newstate();
    /* TODO: check memory here */
    worker->mailbox = mailbox_create();
    worker->working = 1;
    worker->processed = 0;

    /* create the Director table of actions */
    luaL_openlibs(worker->L);
    create_director_table(worker->L);
    director_set(worker->L, 1, director);
    lua_setglobal(worker->L, "Director");

    lua_newtable(worker->L);
    lua_setglobal(worker->L, "__actors_reference");

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
 * Save an Actor's pointer to a table in the Worker's Lua state so that it can
 * be easily cleaned up.
 */
void
worker_save_actor (lua_State *W, Actor *actor)
{
    lua_getglobal(W, "__actors_reference");
    lua_pushlightuserdata(W, actor);
    lua_rawseti(W, -2, luaL_len(W, -2) + 1);
    lua_pop(W, 1);
}

/*
 * Wait for the Worker to wait for work, then join it back to the main thread.
 */
void
worker_stop (Worker *worker)
{
    worker->working = 0;
    pthread_join(worker->thread, NULL);
}

/*
 * Frees the worker. Frees any Actors it created.
 */
void
worker_cleanup (Worker *worker)
{
    int i, len;
    lua_State *W = worker->L;

    printf("%p processed %d\n", worker, worker->processed);

    lua_getglobal(W, "__actors_reference");
    len = luaL_len(W, -1);

    for (i = 1; i <= len; i++) {
        lua_rawgeti(W, -1, i);
        actor_destroy(lua_touserdata(W, -1));
        lua_pop(W, 1);
    }

    mailbox_destroy(worker->mailbox);
    lua_close(worker->L);
    free(worker);
}
