#ifndef DIALOGUE_POSTMAN
#define DIALOGUE_POSTMAN

#include <pthread.h>

/*
 * The Postman is the representation of a thread.
 */

typedef struct Postman {
    pthread_t thread;
    int delivering;
    int has_address;
    struct Mailbox *mailbox;
    struct Actor *address;
} Postman;

Postman *
postman_create (struct Mailbox*);

void
postman_give_address (Postman*, struct Actor*);

void
postman_free (Postman*);

#endif
