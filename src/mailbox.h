#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include "dialogue.h"
#include "worker.h"

typedef struct Mailbox Mailbox;

Mailbox *
mailbox_create (lua_State *L, Worker *worker);

/*
 * Try to push onto the Mailbox's stack. If the stack is full or busy, it 
 * returns 0. Else, it pops the top element off the given Lua stack and pushes
 * it onto the Mailbox's stack and returns 1.
 */
int
mailbox_push_top (lua_State *L, Mailbox *mailbox);

/*
 * Pop all of the actions onto the given Lua stack.
 * Returns the number of actions pushed onto the given Lua stack.
 */
int
mailbox_pop_all (lua_State *L, Mailbox *mailbox);

void
mailbox_destroy (lua_State *L, Mailbox *mailbox);

#endif
