#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "mailbox.h"
#include "utils.h"

/*
 * The Mailbox holds its own Lua stack just for the stack itself. It holds the
 * messages for a Worker and it just pops them off when done with them.
 */
struct Mailbox {
    lua_State *L;
    pthread_mutex_t mutex;
};

Mailbox *
mailbox_create ()
{
    Mailbox *mailbox = malloc(sizeof(*mailbox));
    /* TODO check mailbox NULL */
    mailbox->L = luaL_newstate();
    /* TODO check state NULL */
    pthread_mutex_init(&mailbox->mutex, NULL);
    return mailbox;
}

/*
 * Attempt to pop & push the top element of `L' to the Mailbox's stack. If the
 * Mailbox is busy, returns 0. If the Mailbox cannot handle anymore messages,
 * returns 0. Returns 1 if the top element was popped and pushed.
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

    utils_transfer(B, L, 1);
    ret = 1;

cleanup:
    pthread_mutex_unlock(&mailbox->mutex);
    return ret;
}

/*
 * Pop all of the Mailbox's elements onto `L'. Returns the number of elements
 * pushed.
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
        utils_transfer(L, B, count);
    }

cleanup:
    pthread_mutex_unlock(&mailbox->mutex);
    return count;
}

void
mailbox_destroy (Mailbox *mailbox)
{
    pthread_mutex_lock(&mailbox->mutex);
    if (lua_gettop(mailbox->L) > 0)
        printf("%p quit with %d left\n", mailbox, lua_gettop(mailbox->L));
    pthread_mutex_unlock(&mailbox->mutex);
    pthread_mutex_destroy(&mailbox->mutex);
    lua_close(mailbox->L);
    free(mailbox);
}
