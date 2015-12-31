#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

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

int 
luaopen_Dialogue_Actor (lua_State *L);

#endif
