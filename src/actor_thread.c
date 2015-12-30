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
    pthread_mutex_lock(&actor->action_mutex);
    actor->action = action;
    pthread_mutex_unlock(&actor->action_mutex);
    pthread_cond_signal(&actor->new_action);
}

/*
 * A blocking function which returns the current action and sets the Actor's
 * action to WAIT, so it can do the next action.
 */
Action
actor_next_action (Actor *actor)
{
    Action action;
    pthread_mutex_lock(&actor->action_mutex);
    action = actor->action;
    actor->action = WAIT;
    pthread_mutex_unlock(&actor->action_mutex);
    return action;
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
    Actor *author;
    Mailbox *mailbox = actor->mailbox;
    lua_State *A = actor->L;
    lua_State *B = mailbox->L;
    pthread_mutex_lock(&mailbox->mutex);

    if (mailbox->envelope_count <= 0)
        goto cleanup;

    while (mailbox->envelope_count > 0) {
        if (mailbox == NULL)
            printf("Mailbox is NULL\n");
        else if (mailbox->L == NULL)
            printf("Mailbox stack is NULL\n");
        author = mailbox_push_next_envelope(mailbox);
        utils_copy_top(A, B);
        lua_pop(B, 1);

        if (lua_isnil(A, -1))
            goto next;

        for (script = actor->script_head; script != NULL; script = script->next)
            if (script->is_loaded)
                script_send(script, author);
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
    Action action;
    Actor *actor = arg;
    pthread_mutex_lock(&actor->state_mutex);

    while (actor->on) {
        action = actor_next_action(actor);
        switch (action) {
        case LOAD:
            actor_load_scripts(actor);
            break;

        case RECEIVE:
            actor_process_mailbox(actor);
            break;

        case STOP:
            goto exit;
            break;

        case WAIT:
            pthread_cond_wait(&actor->new_action, &actor->state_mutex);
            break;

        default:
            break;
        }
    }

exit:
    pthread_mutex_unlock(&actor->state_mutex);
    return NULL;
}
