#ifndef DIALOGUE_POST_POSTMAN
#define DIALOGUE_POST_POSTMAN

#include "mailbox.h"
#include <pthread.h>

typedef struct Postman {
    lua_State *L; pthread_t thread;
    int working;
    struct Mailbox *mailbox;
} Postman;
/*
 * If the postman's bag still has envelopes, do nothing. Otherwise, wait for 
 * the mailbox to be free and then fill the bag with envelopes from the mailbox.
 */
void
postman_fill_bag (Postman *postman);

Postman *
postman_create ();

void
postman_stop (Postman *postman);

#endif
