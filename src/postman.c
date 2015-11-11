#include <stdlib.h>
#include <unistd.h>
#include "mailbox.h"
#include "postman.h"
#include "actor.h"

/*
 * Deliver the next message to the given address
 */
void
postman_deliver (Postman *postman)
{
    //lua_State *B = mailbox_request_stack(postman->mailbox);
    //mailbox_return_stack(postman->mailbox);
}

void *
postman_thread (void *arg)
{
    Postman *postman = arg;

    while (postman->delivering)
        while (postman->has_address)
            postman_deliver(postman);

    return NULL;
}

/*
 * 
 */
void
postman_create (lua_State *L, Postman *postman, Mailbox *mailbox)
{
    pthread_t thread;

    postman = malloc(sizeof(Postman));

    if (postman == NULL)
        luaL_error(L, "Error allocating memory for Postman thread!");

    postman->delivering = 1;
    postman->has_address = 0;
    postman->address = NULL;
    postman->mailbox = mailbox;

    //pthread_create(&thread, NULL, postman_thread, postman);
    //pthread_detach(thread);
}

/*
 * Assign an address for the postman to deliver envelopes to.
 */
void
postman_give_address (Postman *postman, Actor *address)
{
    postman->address = address;
    postman->has_address = 1;
}

/*
 * Tell the postman to stop and wait for its to fully end.
 */
void
postman_free (Postman *postman)
{
    postman->delivering = 0;
    postman->has_address = 0;
    postman->address = NULL;
    postman->mailbox = NULL;
    usleep(2000);
    free(postman);
    postman = NULL;
}
