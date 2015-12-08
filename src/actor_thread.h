#ifndef DIALOGUE_ACTOR_THREAD
#define DIALOGUE_ACTOR_THREAD

#include <lua.h>
#include <pthread.h>
#include <unistd.h>

struct Actor;

typedef enum Action {
    LOAD, RECEIVE, SEND, WAIT, STOP
} Action;

/*
 * Request access to the Actor's state with a convinient return of the stack.
 */
lua_State *
actor_request_state (struct Actor *actor);

/*
 * Return access so it can give it away again.
 */
void
actor_return_state (struct Actor *actor);

/*
 * Set the action for the given Actor and alert it of its new action.
 */
void
actor_alert_action (struct Actor *actor, Action action);

/*
 * The Actor's thread which handles receiving messages, loading its initial 
 * state.
 */
void *
actor_thread (void *arg);

#endif
