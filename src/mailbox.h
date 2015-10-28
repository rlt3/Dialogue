#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>
#include "envelope.h"

#define MAILBOX_LIB "Dialogue.Mailbox"

typedef struct Mailbox {
    lua_State *L;
    pthread_mutex_t mutex;
    int envelopes_ref;
    int ref;
} Mailbox;

/*
 * Make sure the argument at index N is a Mailbox and return it if it is.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index);

int 
luaopen_Dialogue_Mailbox (lua_State * L);

#endif
