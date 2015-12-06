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
 * Set the action for the given Actor and alert it of its new action.
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
 * The Actor's thread which handles receiving messages, loading its initial 
 * state.
 */
void *
actor_thread (void *arg)
{
    Actor *actor = arg;

    pthread_mutex_lock(&actor->state_mutex);

    while (actor->on) {
        switch (actor->action) {
        case LOAD:
            break;

        case RECEIVE:
            break;

        case SEND:
            break;

        default:
            pthread_cond_wait(&actor->new_action, &actor->state_mutex);
            break;
        }
    }

    return NULL;
}
