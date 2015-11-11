#ifndef DIALOGUE_POSTMAN
#define DIALOGUE_POSTMAN

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>

/*
 * The Postman is the representation of a thread.
 */

typedef struct Postman {
    int delivering;
    int has_address;
    struct Mailbox *mailbox;
    struct Actor *address;
} Postman;

/*
 * 
 */
void
postman_create (lua_State *L, Postman *postman, struct Mailbox *mailbox);

/*
 * Assign an address for the postman to deliver envelopes to.
 */
void
postman_give_address (Postman *postman, struct Actor *address);

/*
 * Tell the postman to stop and wait for its to fully end.
 */
void
postman_free (Postman *postman);

#endif
