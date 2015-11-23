#ifndef DIALOGUE_POSTMAN
#define DIALOGUE_POSTMAN

#include "mailbox.h"

typedef struct Postman {
    lua_State *L;
    int delivering;
    int has_work;
    Mailbox *mailbox;
    pthread_mutex_t mutex;
    pthread_cond_t get_work;
    pthread_t thread;
} Postman;

/*
 * Create a postman which waits for the mailbox to tell it when to get a new
 * envelope and deliver it. Returns pointer if OK or NULL if not.
 */
Postman *
postman_new (Mailbox *mailbox);

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
