#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "actor_thread.h"

#define ACTOR_LIB "Dialogue.Actor"

typedef struct Actor {
    lua_State *L;

    /* 
     * A read write lock for the structure of the Actor -- it's place in the
     * Dialogue tree -- it's parent, siblings, and children.
     */
    pthread_rwlock_t structure;

    /* For everything else including the state & scripts */
    pthread_mutex_t state_mutex;

    /*
     * Restrict to single thread. If is_star is true, then restrict to main 
     * thread. If is_lead is true, then restrict to a Postman thread.
     */
    int is_lead;
    int is_star;

    /* Tree nav: go up, horizontally, and down the tree */
    struct Actor *parent;
    struct Actor *next;
    struct Actor *child;

    /* Go directly to the head of the tree */
    struct Actor *dialogue;

    struct Script *script_head;
    struct Script *script_tail;

    /* If a Mailbox is set, the Post will exclusively send envelopes to it */
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
 * Remove the Script from the Actor's linked-list.
 */
void
actor_remove_script (Actor *actor, struct Script *script);

lua_State *
actor_request_state (Actor *actor);

void
actor_return_state (Actor *actor);

int
actor_is_calling_thread (pthread_t pid);

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index);

int 
luaopen_Dialogue_Actor (lua_State *L);

#endif
