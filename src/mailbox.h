#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include "actor.h"

struct Mailbox *
mailbox_create ();

void
mailbox_destroy (struct Mailbox *mailbox);

/*
 * This is a blocking function. Wait for the Mailbox and then copy the expected
 * Envelope on top of the given Lua stack. Adds the envelope to the queue.
 * Returns 1 if the envelope was sent, 0 if busy.
 */
int
mailbox_send_lua_top (struct Mailbox *mailbox, lua_State *L);

/*
 * Pops all of the Mailbox's envelopes (a destructive operation) as a table
 * and pushes it onto the given Lua state. Returns the number of envelopes
 * inside the table.
 */
int
mailbox_pop_envelopes (struct Mailbox *mailbox, lua_State *L);

#endif
