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
 * Add an envelope to our mailbox.
 */
void
mailbox_add (Mailbox *box, Envelope *envelope);

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
