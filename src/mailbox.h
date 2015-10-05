#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include "envelope.h"

#define MAILBOX_LIB "Dialogue.Mailbox"

typedef struct Mailbox {
    Envelope *head;
    Envelope *tail;
    int processing;
} Mailbox;

/*
 * A thread platform for having our mailbox run concurrently.
 */
void *
mailbox_thread (void *arg);

/* 
 * Allocate space for the envelope in the stream.
 */
Envelope *
mailbox_stream_allocate (Envelope envelope);

/*
 * Free an envelope from the stream and return it. If the stream pointer given
 * is NULL, this returns an empty envelope that will fail a bind call.
 */
Envelope
mailbox_stream_retrieve (Envelope *envelope);

/*
 * Add an envelope to our mailbox.
 */
void
mailbox_add (Mailbox *box, Envelope envelope);

/*
 * Return the next Envelope.
 */
Envelope
mailbox_next (Mailbox *box);

/*
 * Make sure the argument at index N is a Mailbox and return it if it is.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index);

#endif
