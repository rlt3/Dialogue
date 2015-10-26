#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>

#define ACTOR_LIB "Dialogue.Actor"

struct Envelope;

typedef struct Actor {
    lua_State *L;

    pthread_mutex_t mutex;
    pthread_mutex_t stack_mutex;

    struct Actor *parent;
    struct Actor *next;
    struct Actor *child;

    struct Script *script;

    struct Actor *dialogue;
    struct Mailbox *mailbox;

    int ref;
} Actor;

/*
 * Add a script to the given actor, always at the front.
 */
void
actor_add_script (Actor *actor, struct Script *script);

/*
 * Add a child to the given actor, always at the front.
 */
void
actor_add_child (Actor *actor, Actor *child);

/*
 * From an envelope, send a message to each Script an actor owns.
 */
void
actor_send_envelope (Actor *actor, struct Envelope *envelope);

/*
 * Find and remove the Script from the Actor's linked-list of Scripts.
 */
void
actor_remove_script (Actor *actor, struct Script *removed);

/*
 * Request the stack from the given actor so that we can have thread-safe apis.
 */
lua_State*
actor_request_stack (Actor *actor);

/*
 * Return the stack back to the Actor who can give it out again.
 */
void
actor_return_stack (Actor *actor);

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index);

int 
luaopen_Dialogue_Actor (lua_State *L);

#endif
