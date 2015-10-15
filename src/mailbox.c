#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "mailbox.h"
#include "post.h"
#include "utils.h"

/*
 * A thread platform for having our mailbox run concurrently.
 */
void *
mailbox_thread (void *arg)
{
    Mailbox *box = (Mailbox*) arg;

    while (box->processing) {
        /* Bind (or Just Envelope) the next envelope to the post function */
        envelope_bind(mailbox_next(box), post);
        usleep(10000);
    }

    return NULL;
}

/*
 * Free an envelope from the stream and return it. If the stream pointer given
 * is NULL, this returns an empty envelope that will fail a bind call.
 */
Envelope
mailbox_stream_retrieve (Envelope *stream)
{
    Envelope envelope;

    if (stream == NULL)
        return envelope_create_empty();

    /* free the envelope, but not the malloc'd data array inside */
    envelope = *stream;
    free(stream);

    return envelope;
}

/*
 * Take ownership of the Envelope pointer and add it to the Stream.
 */
void
mailbox_add (Mailbox *box, Envelope *envelope)
{
    if (box->head == NULL) {
        box->head = envelope;
        box->tail = envelope;
    } else {
        box->tail->next = envelope;
        box->tail = envelope;
    }
}

/*
 * Return the next Envelope.
 */
Envelope
mailbox_next (Mailbox *box)
{
    Envelope envelope = mailbox_stream_retrieve(box->head);
    box->head = envelope.next;
    envelope.next = NULL;
    return envelope;
}

/*
 * Make sure the argument at index N is a Mailbox and return it if it is.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index)
{
    return (Mailbox*) luaL_checkudata(L, index, MAILBOX_LIB);
}

/*
 * This spawns a thread so the mailbox can process envelopes concurrently to
 * the interpreter. No thread is spawned if allocation fails.
 */
static int
lua_mailbox_new (lua_State *L)
{
    pthread_t thread;

    Mailbox *box = lua_newuserdata(L, sizeof(Mailbox));
    luaL_getmetatable(L, MAILBOX_LIB);
    lua_setmetatable(L, -2);

    box->head = NULL;
    box->tail = NULL;
    box->processing = 1;

    pthread_create(&thread, NULL, mailbox_thread, box);
    pthread_detach(thread);

    return 1;
}

static int
lua_mailbox_print (lua_State *L)
{
    Mailbox *box = lua_check_mailbox(L, 1);
    lua_pushfstring(L, "%s %p", MAILBOX_LIB, box);
    return 1;
}

/*
 * Stop thread and free any envelopes before Lua garbage collects the mailbox.
 */
static int
lua_mailbox_gc (lua_State *L)
{
    Mailbox *box = lua_check_mailbox(L, 1);
    box->processing = 0;

    /* wait for thread to destruct */
    usleep(10000);

    /* `mailbox_next` frees the stream objects as it gets to them. */
    while (box->head != NULL)
        mailbox_next(box);

    luaL_unref(L, LUA_REGISTRYINDEX, box->ref);

    return 0;
}

static const luaL_Reg mailbox_methods[] = {
    {"__tostring", lua_mailbox_print},
    {"__gc",       lua_mailbox_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Mailbox (lua_State * L)
{
    return lua_meta_open(L, MAILBOX_LIB, mailbox_methods, lua_mailbox_new);
}
