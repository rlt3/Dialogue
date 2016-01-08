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
    int action_count;
    int ref;
};

Mailbox *
mailbox_create (lua_State *L)
{
    Mailbox *mailbox = malloc(sizeof(*mailbox));

    //if (mailbox == NULL)

    mailbox->L = lua_newthread(L);
    mailbox->action_count = 0;
    mailbox->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    pthread_mutex_init(&mailbox->mutex, NULL);

    return mailbox;
}

/*
 * Try pushing to the mailbox. Pops the top of the Lua stack and pushes it to
 * the Mailbox if it isn't busy.  Returns 1 if the action is taken, 0 if busy.
 */
int
mailbox_push_top (lua_State *L, Mailbox *mailbox)
{
    int rc;
    lua_State *B = mailbox->L;

    rc = pthread_mutex_trylock(&mailbox->mutex);

    if (rc == EBUSY)
        return 0;

    lua_xmove(L, B, 1);
    mailbox->action_count++;
    pthread_mutex_unlock(&mailbox->mutex);

    return 1;
}

/*
 * Pop all of the actions off the Mailbox onto the given Lua stack.
 */
void
mailbox_pop_all (lua_State *L, Mailbox *mailbox)
{
    lua_State *B = mailbox->L;

    pthread_mutex_lock(&mailbox->mutex);
    lua_xmove(B, L, mailbox->action_count);
    mailbox->action_count = 0;
    pthread_mutex_unlock(&mailbox->mutex);
}

void
mailbox_destroy (lua_State *L, Mailbox *mailbox)
{
    luaL_unref(L, LUA_REGISTRYINDEX, mailbox->ref);
    free(mailbox);
}
