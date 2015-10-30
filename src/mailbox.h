#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>

#define MAILBOX_LIB "Dialogue.Mailbox"

typedef struct Mailbox {
    lua_State *L;
    int envelope_count;
    int processing;
    pthread_mutex_t mutex;
    struct Postman **postmen;
    int postmen_count;
    int envelopes_ref;
    int ref;
} Mailbox;

/*
 * Make sure the argument at index N is a Mailbox and return it if it is.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index);

lua_State *
mailbox_request_stack (Mailbox*);

void
mailbox_return_stack (Mailbox*);

int 
luaopen_Dialogue_Mailbox (lua_State * L);

#endif
