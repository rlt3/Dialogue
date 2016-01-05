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
 * The Mailbox has its own state and mutex. It collects the messages for an
 * Actor while the Actor does other things. When an Actor is ready to process
 * the messages, the Mailbox locks, the Actor locks, and they processed until
 * there are no messages.
 */
typedef struct Mailbox {
    lua_State *L;
    pthread_mutex_t mutex;
    int envelope_count;
    int ref;
} Mailbox;

Mailbox *
mailbox_create ()
{
    lua_State *B;
    pthread_mutexattr_t mutex_attr;
    Mailbox *mailbox = malloc(sizeof(*mailbox));

    mailbox->envelope_count = 0;
    mailbox->L = luaL_newstate();

    B = mailbox->L;
    luaL_openlibs(B);

    /*
     * TODO:
     *  *if* we treat each Envelope as simply a table in the form of
     *      { author, action [, arg1 [, arg2 ...]] }
     *  then we can simply loop through the table and for each slot that could
     *  contain an actor, we check for the existence of a 'reference' field. If
     *  it exists, we call it as a function (incrementing whatever it may be).
     */

    luaL_requiref(B, "eval", luaopen_eval, 1);
    luaL_requiref(B, "Collection", luaopen_Collection, 1);
    luaL_requiref(B, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(B, 3);

    lua_newtable(B);
    lua_setglobal(B, "__envelopes");

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mailbox->mutex, &mutex_attr);

    return mailbox;
}

void
mailbox_destroy (Mailbox *mailbox)
{
    free(mailbox);
}

/*
 * This is a blocking function. Wait for the Mailbox and then copy the expected
 * Envelope on top of the given Lua stack. Adds the envelope to the queue.
 * Returns 1 if the envelope was sent, 0 if busy.
 */
int
mailbox_send_lua_top (Mailbox *mailbox, lua_State *L)
{
    int rc;
    lua_State *B = mailbox->L;

    rc = pthread_mutex_trylock(&mailbox->mutex);
    if (rc == EBUSY)
        return 0;

    lua_getglobal(B, "__envelopes");
    utils_copy_top(B, L);
    lua_rawseti(B, -2, mailbox->envelope_count + 1);
    lua_pop(B, 1);
    mailbox->envelope_count++;
    pthread_mutex_unlock(&mailbox->mutex);

    return 1;
}

/*
 * Pops all of the Mailbox's envelopes (a destructive operation) as a table
 * and pushes it onto the given Lua state. Returns the number of envelopes
 * inside the table.
 */
int
mailbox_pop_envelopes (Mailbox *mailbox, lua_State *L)
{
    int count;
    lua_State *B = mailbox->L;

    pthread_mutex_lock(&mailbox->mutex);
    luaf(B, "return __envelopes", 1);
    utils_copy_top(L, B);
    lua_pop(B, 1);
    luaf(B, "__envelopes = Collection{}");
    count = mailbox->envelope_count;
    mailbox->envelope_count = 0;
    pthread_mutex_unlock(&mailbox->mutex);

    return count;
}
