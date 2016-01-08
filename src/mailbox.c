#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "dialogue.h"
#include "luaf.h"
#include "collection.h"
#include "mailbox.h"
#include "utils.h"
#include "luaf.h"

/*
 * The Mailbox holds its own Lua stack just for the stack itself. It holds the
 * messages for a Worker and it just pops them off when done with them.
 */
struct Mailbox {
    lua_State *L;
    pthread_mutex_t mutex;
    int ref;
};

Mailbox *
mailbox_create (lua_State *L)
{
    Mailbox *mailbox = malloc(sizeof(*mailbox));
    //if (mailbox == NULL)
    mailbox->L = lua_newthread(L);
    mailbox->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    pthread_mutex_init(&mailbox->mutex, NULL);

    return mailbox;
}

/*
 * Try to push onto the Mailbox's stack. If the stack is full or busy, it 
 * returns 0. Else, it pops the top element off the given Lua stack and pushes
 * it onto the Mailbox's stack and returns 1.
 */
int
mailbox_push_top (lua_State *L, Mailbox *mailbox)
{
    int rc, ret = 0;
    lua_State *B = mailbox->L;

    rc = pthread_mutex_trylock(&mailbox->mutex);

    if (rc == EBUSY)
        return 0;

    if (!lua_checkstack(B, 1))
        goto cleanup;

    lua_xmove(L, B, 1);
    ret = 1;

cleanup:
    pthread_mutex_unlock(&mailbox->mutex);
    return ret;
}

/*
 * Pop all of the actions onto the given Lua stack.
 * Returns the number of actions pushed onto the given Lua stack.
 */
int
mailbox_pop_all (lua_State *L, Mailbox *mailbox)
{
    int count;
    lua_State *B = mailbox->L;

    pthread_mutex_lock(&mailbox->mutex);
    count = lua_gettop(B);

    if (count > 0) {
        if (!lua_checkstack(L, count)) {
            count = 0;
            goto cleanup;
        }
        lua_xmove(B, L, count);
    }

cleanup:
    pthread_mutex_unlock(&mailbox->mutex);
    return count;
}

void
mailbox_destroy (lua_State *L, Mailbox *mailbox)
{
    pthread_mutex_lock(&mailbox->mutex);
    if (lua_gettop(mailbox->L) > 0)
        printf("%p quit with %d left\n", mailbox, lua_gettop(mailbox->L));
    pthread_mutex_unlock(&mailbox->mutex);
    pthread_mutex_destroy(&mailbox->mutex);
    free(mailbox);
}
