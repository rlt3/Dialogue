#include <stdio.h>
#include "actor_thread.h"
#include "actor.h"
#include "utils.h"

/*
 * Request access to the Lua stack from the Actor (and only the stack).
 */
lua_State *
actor_request_stack (Actor *actor)
{
    /*
     * TODO: What if something is waiting on the Lua stack after the Actor
     * gets gc'd? Error codes or check for NULL?
     */
    pthread_mutex_lock(&actor->stack_mutex);
    return actor->L;
}

/*
 * Return the stack back to the Actor who can give it out again.
 */
void
actor_return_stack (Actor *actor)
{
    pthread_mutex_unlock(&actor->stack_mutex);
}

/*
 * Request access to the Actor's state (but not the Lua stack).
 */
void
actor_request_state (Actor *actor)
{
    pthread_mutex_lock(&actor->state_mutex);
}

/*
 * Return access to the Actor's state.
 */
void
actor_return_state (Actor *actor)
{
    pthread_mutex_unlock(&actor->state_mutex);
}

/*
 * A blocking function which alerts the Actor to its new action.
 */
void
actor_alert_action (Actor *actor, Action action)
{
    actor_request_state(actor);
    actor->action = action;
    pthread_cond_signal(&actor->new_action);
    actor_return_state(actor);
}

/*
 * The Actor's thread which handles receiving messages, loading its initial 
 * state.
 */
void *
actor_thread (void *arg)
{
    Actor *actor = arg;

    printf("Actor thread %p: starting...\n", actor);

    pthread_mutex_lock(&actor->state_mutex);

    while (actor->on) {
        switch (actor->action) {
        case LOAD:
            /*
             * We must handle loading of Scripts because many modules require
             * all functions be called from a single thread.
             */
            printf("Actor thread %p: loading...\n", actor);
            actor->action = WAIT;
            break;

        case RECEIVE:
            /*
             * We must handle execution of the Script's methods as per above.
             */
            printf("Actor thread %p: receiving...\n", actor);
            actor->action = WAIT;
            break;

        case SEND:
            /* 
             * We can use this thread to send to other threads though
             * actors. Or we can still have an Postman pool & Mailbox to make
             * sure throughput of the core feature is not bog down.
             */
            printf("Actor thread %p: sending...\n", actor);
            actor->action = WAIT;
            break;

        case STOP:
            printf("Actor thread %p: quitting...\n", actor);
            goto exit;
            break;

        default:
            printf("Actor thread %p: waiting...\n", actor);
            pthread_cond_wait(&actor->new_action, &actor->state_mutex);
            break;
        }
    }

exit:
    return NULL;
}
