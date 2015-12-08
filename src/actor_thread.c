#include <stdio.h>
#include "actor.h"
#include "mailbox.h"
#include "script.h"
#include "utils.h"

/*
 * Request access to the Actor's state with a convinient return of the stack.
 */
lua_State *
actor_request_state (Actor *actor)
{
    /*
     * TODO: What if something is waiting on the Lua stack after the Actor
     * gets gc'd? Error codes or check for NULL?
     */
    pthread_mutex_lock(&actor->state_mutex);
    return actor->L;
}


/*
 * Return access so it can give it away again.
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
    actor_return_state(actor);
    pthread_cond_signal(&actor->new_action);
}

/*
 * Load each Script an Actor owns.
 */
void
actor_load_scripts (Actor *actor)
{
    Script *script;

    /* 
     * Remember: we already have the state mutex & script_load asks for the 
     * stack mutex as well
     */
    for (script = actor->script_head; script != NULL; script = script->next)
        if (script->be_loaded)
            script_load(script);
}

/*
 * Assumes the Actor mutex is acquired. Blocks for the Mailbox mutex. Processes
 * all envelopes currently held by the Mailbox.
 */
void
actor_process_mailbox (Actor *actor)
{
    Script *script;
    Mailbox *mailbox = actor->mailbox;
    lua_State *A = actor->L;
    lua_State *B = mailbox->L;
    pthread_mutex_lock(&mailbox->mutex);

    if (mailbox->envelope_count <= 0)
        goto cleanup;

    while (mailbox->envelope_count > 0) {
        mailbox_push_next_envelope(mailbox);
        utils_copy_top(A, B);
        lua_pop(B, 1);

        if (lua_isnil(A, -1))
            goto next;

        for (script = actor->script_head; script != NULL; script = script->next)
            if (script->is_loaded)
                script_send(script);
next:
        lua_pop(A, 1);
    }

cleanup:
    pthread_mutex_unlock(&mailbox->mutex);
}

/*
 * Block for the Actor's state and call the appropriate function for the given
 * Action. This is for doing actions in a specific thread rather than telling
 * the Actor to process in its own thread.
 */
void
actor_call_action (Actor *actor, Action action)
{
    actor_request_state(actor);
    switch (action) {
    case LOAD:
        actor_load_scripts(actor);
        break;

    case RECEIVE:
        actor_process_mailbox(actor);
        break;

    default:
        break;
    }
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
            actor_load_scripts(actor);
            actor->action = WAIT;
            break;

        case RECEIVE:
            /*
             * We must handle execution of the Script's methods as per above.
             */
            printf("Actor thread %p: receiving...\n", actor);
            actor_process_mailbox(actor);
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
    pthread_mutex_unlock(&actor->state_mutex);
    return NULL;
}
