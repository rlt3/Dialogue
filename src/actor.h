#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "actor_thread.h"

#define ACTOR_LIB "Dialogue.Actor"

/*
 * An empty container defined by the Scripts it owns. It has its own thread
 * which handles all the operations of its Scripts including receiving messages.
 */
typedef struct Actor {
    lua_State *L;

    pthread_t thread;

    /* for anything about the actor other than the Mailbox & structure */
    pthread_mutex_t state_mutex;

    /* Action is the condition to be set to tell the thread what to do */
    pthread_cond_t new_action;
    Action action;

    short int on;
    short int manual_call;

    /* Tree nav: go up, horizontally, and down the tree */
    struct Actor *parent;
    struct Actor *next;
    struct Actor *child;

    /* Go directly to the head of the tree */
    struct Actor *dialogue;

    struct Script *script_head;
    struct Script *script_tail;

    struct Mailbox *mailbox;

    /* A place for a parent to set their reference */
    int ref;
} Actor;

/*
 * Add the Script to the end of the Actor's linked-list of Scripts.
 */
void
actor_add_script (Actor *actor, struct Script *script);

/*
 * Remove the Script and connect the previous to the next.
 */
void
actor_remove_script (Actor *actor, struct Script *script);

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index);

int 
luaopen_Dialogue_Actor (lua_State *L);

#endif
