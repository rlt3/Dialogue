#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include "actor.h"

#define MAILBOX_LIB "Dialogue.Actor.Mailbox"

/*
 * The Mailbox has its own state and mutex. It collects the messages for an
 * Actor while the Actor does other things. When an Actor is ready to process
 * the messages, the Mailbox locks, the Actor locks, and they processed until
 * there are no messages.
 */
typedef struct Mailbox {
    lua_State *L;
    Actor *actor;
    pthread_mutex_t mutex;
    int envelope_count;
    int ref;
} Mailbox;

/*
 * Check for a Mailbox at index. Errors if it isn't a Mailbox.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index);

/*
 * Block for the Mailbox. Expects a message on top of given stack copy message
 * from top of stack to the Mailbox's queue.
 */
void
mailbox_send (Mailbox *mailbox, lua_State *L);

/*
 * Assumes Mailbox mutex is acquired. Removes the next envelope from its queue
 * of Envelopes and leaves it at the top of the Mailbox stack. Can leave nil if
 * no envelopes.
 */
void
mailbox_push_next_envelope (Mailbox *mailbox);

int 
luaopen_Dialogue_Actor_Mailbox (lua_State *L);

#endif
