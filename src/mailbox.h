#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include "dialogue.h"
#include "worker.h"

typedef struct Mailbox Mailbox;

Mailbox *
mailbox_create ();

/*
 * Attempt to pop & push the top element of `L' to the Mailbox's stack. If the
 * Mailbox is busy, returns 0. If the Mailbox cannot handle anymore messages,
 * returns 0. Returns 1 if the top element was popped and pushed.
 */
int
mailbox_push_top (lua_State *L, Mailbox *mailbox);

/*
 * Pop all of the Mailbox's elements onto `L'. Returns the number of elements
 * pushed.
 */
int
mailbox_pop_all (lua_State *L, Mailbox *mailbox);

void
mailbox_destroy (Mailbox *mailbox);

#endif
