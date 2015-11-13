#ifndef DIALOGUE_MAILBOX
#define DIALOGUE_MAILBOX

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>

#define MAILBOX_LIB "Dialogue.Mailbox"

typedef struct Mailbox {
    lua_State *L;
    int processing;
    pthread_mutex_t mutex;
    pthread_cond_t new_envelope;
    pthread_t thread;
    struct Postman **postmen;
    int postmen_count;
    int envelopes_table;
    int envelope_count;
    int ref;
} Mailbox;

/*
 * Make sure the argument at index N is a Mailbox and return it if it is.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index);

/*
 * Removes the next envelope from its queue of Envelopes and leaves it at the top
 * of the Mailbox stack. Must have a lock on the Mailbox first.
 */
void
mailbox_push_next_envelope (Mailbox *mailbox);

int 
luaopen_Dialogue_Mailbox (lua_State * L);

#endif
