#ifndef MAILBOX_H
#define MAILBOX_H

#define MAILBOX_LIB "Mailbox"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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
} Mailbox;

/*
 * Removes the next envelope from its queue of Envelopes and leaves it at the top
 * of the Mailbox stack.
 */
void
mailbox_push_next_envelope (Mailbox *mailbox);

#endif
