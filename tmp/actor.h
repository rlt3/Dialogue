#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "actor_thread.h"

#define ACTOR_LIB "Dialogue.Actor"

typedef struct Actor {
    /* for accessing the stack specifically */
    pthread_mutex_t stack_mutex;
    lua_State *L;

    /* for anything about the actor other than stack */
    pthread_mutex_t state_mutex;
    pthread_cond_t new_action;
    pthread_t thread;
    Action action;
    short int on;

    struct Actor *parent;
    struct Actor *next;
    struct Actor *child;

    struct Script *script;

    struct Actor *dialogue;
    struct Mailbox *mailbox;

    int ref;
} Actor;

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index);

#endif
