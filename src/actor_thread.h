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
 * Request access to the Lua stack from the Actor (and only the stack).
 */
lua_State *
actor_request_stack (struct Actor *actor);

/*
 * Return the stack back to the Actor who can give it out again.
 */
void
actor_return_stack (struct Actor *actor);

/*
 * Request access to the Actor's state (but not the Lua stack).
 */
void
actor_request_state (struct Actor *actor);

/*
 * Return access to the Actor's state.
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
