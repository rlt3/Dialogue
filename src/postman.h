#ifndef DIALOGUE_POST_POSTMAN
#define DIALOGUE_POST_POSTMAN

#include "mailbox.h"
#include <pthread.h>

typedef struct Postman {
    lua_State *L; pthread_t thread;
    int working;
    long int messages_processed;
    struct Mailbox *mailbox;
    int ref;
} Postman;
/*
 * If the postman's bag still has envelopes, do nothing. Otherwise, wait for 
 * the mailbox to be free and then fill the bag with envelopes from the mailbox.
 */
void
postman_fill_bag (Postman *postman);

Postman *
postman_create (lua_State *L, void *post);

void
postman_stop (Postman *postman);

#endif
