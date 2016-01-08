#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include "dialogue.h"

typedef struct Mailbox Mailbox;

Mailbox *
mailbox_create (lua_State *L);

/*
 * Try pushing to the mailbox. Pops the top of the Lua stack and pushes it to
 * the Mailbox if it isn't busy.  Returns 1 if the action is taken, 0 if busy.
 */
int
mailbox_push_top (lua_State *L, Mailbox *mailbox);

/*
 * Pop all of the actions off the Mailbox onto the given Lua stack.
 */
void
mailbox_pop_all (lua_State *L, Mailbox *mailbox);

void
mailbox_destroy (lua_State *L, Mailbox *mailbox);

#endif
