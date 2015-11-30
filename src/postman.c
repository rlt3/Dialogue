#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "dialogue.h"
#include "postman.h"
#include "envelope.h"
#include "actor.h"
#include "script.h"
#include "tone.h"
#include "utils.h"

/*
 * Use the Postman's internal stack and load the recipient and call its 'send'
 * with the message table at the given index. When there's an error we simply
 * print it out rather than crashing. The stack is balanced at the end.
 */
void
postman_send (Postman *postman, Actor *author, Actor *recipient, int message_index)
{
    lua_State *P = postman->L;
    utils_push_object(P, recipient, ACTOR_LIB);
    lua_getfield(P, -1, "send");
    lua_pushvalue(P, -2);
    utils_push_object(P, author, ACTOR_LIB);
    lua_pushvalue(P, message_index);
    if (lua_pcall(P, 3, 0, 0))
        printf("%s\n", lua_tostring(P, -1));
    lua_pop(P, 1);
}

/*
 * The Postman gets the next Envelope from the Mailbox and then delivers it by
 * the tone to the correct audience. If the recipient is already set, then send
 * to the recipient and the recipient only.
 */
void
postman_deliver (Postman *postman)
{
    int audience_index;
    Envelope envelope;
    Mailbox *mailbox = postman->mailbox;
    lua_State *B = mailbox->L;
    lua_State *P = postman->L;

    /* 
     * Wait for Mailbox access. We push the next Envelope and get the relevant
     * info and give up access as soon as possible. Other threads could be using
     * that access to deliver other envelopes!
     */

    pthread_mutex_lock(&mailbox->mutex);
    mailbox_push_next_envelope(mailbox);
    envelope = *lua_check_envelope(B, -1);
    envelope_push_message(B, -1);
    utils_copy_top(P, B);
    lua_pop(B, 2);
    pthread_mutex_unlock(&mailbox->mutex);

    /* if there's a recipient set, we're assuming we're sending just to it */
    if (envelope.recipient != NULL) {
        postman_send(postman, envelope.author, envelope.recipient, 1);
        lua_pop(P, 1); /* message table */
        goto cleanup;
    }

    /* Get the author's audience by the tone */
    audience_filter_tone(P, envelope.author, envelope.tone);
    audience_index = lua_gettop(P);
    luaL_checktype(P, audience_index, LUA_TTABLE);

    /* and send each of the actors in the audience the message */
    lua_pushnil(P);
    while (lua_next(P, audience_index)) {
        postman_send(postman, envelope.author, lua_check_actor(P, -1), 1);
        lua_pop(P, 1);
    }

    lua_pop(P, 2); /* audience table, message */

cleanup:
    postman->has_work = 0;
}

void *
postman_thread (void *arg)
{
    int rc;
    Postman *postman = arg;

    rc = pthread_mutex_lock(&postman->mutex);

    while (postman->delivering) {
        if (postman->has_work) {
            postman_deliver(postman);
        } else {
            rc = pthread_cond_wait(&postman->get_work, &postman->mutex);
        }
    }

    return NULL;
}

/*
 * Create a postman which waits for the mailbox to tell it when to get a new
 * envelope and deliver it. Returns pointer if OK or NULL if not.
 */
Postman *
postman_new (Mailbox *mailbox)
{
    lua_State *P;
    pthread_mutexattr_t mutex_attr;

    Postman *postman = malloc(sizeof *postman);

    if (postman == NULL)
        goto exit;

    postman->mailbox = mailbox;
    postman->delivering = 1;
    postman->has_work = 0;
    postman->get_work = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    pthread_mutexattr_init(&mutex_attr);
    //pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&postman->mutex, &mutex_attr);

    postman->L = luaL_newstate();
    P = postman->L;
    luaL_openlibs(P);
    luaL_requiref(P, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(P, 1);

    pthread_create(&postman->thread, NULL, postman_thread, postman);
    pthread_detach(postman->thread);

exit:
    return postman;
}

/*
 * Tell the postman to get an address (the next envelope) from the mailbox.
 * If the postman is busy (already delivering an envelope) then this returns 0.
 * If it wasn't busy it returns 1.
 */
int
postman_get_address (Postman *postman)
{
    if (postman->has_work)
        goto busy;

    postman->has_work = 1;
    pthread_cond_signal(&postman->get_work);

    return 1;
busy:
    return 0;
}

/*
 * Wait for the postman to get done delivering anything and then free him.
 */
void
postman_free (Postman *postman)
{
    int rc = pthread_mutex_lock(&postman->mutex);
    postman->delivering = 0;
    rc = pthread_mutex_unlock(&postman->mutex);

    lua_close(postman->L);
    free(postman);
}
