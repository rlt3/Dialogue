#ifndef POSTMAN_H
#define POSTMAN_H

#include "Mailbox.h"

typedef struct Postman {
    lua_State *L;
    int delivering;
    int needs_address;
    Mailbox *mailbox;
    pthread_mutex_t mutex;
    pthread_cond_t get_address;
    pthread_t thread;
} Postman;

void
postman_new (lua_State *L, Postman *postman, Mailbox *mailbox);

/*
 * Tell the postman to get an address (the next envelope) from the mailbox.
 * If the postman is busy (already delivering an envelope) then this returns 0.
 * If it wasn't busy it returns 1.
 */
int
postman_get_address (Postman *postman);

/*
 * Wait for the postman to get done delivering anything and then free him.
 */
void
postman_free (Postman *postman);

#endif
