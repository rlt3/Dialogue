#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>

#define ACTOR_LIB "Dialogue.Actor"

typedef struct Actor {
    lua_State *L;
    struct Script *script;
    pthread_mutex_t mutex;
} Actor;

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index);

int 
luaopen_Dialogue_Actor (lua_State *L);

#endif
