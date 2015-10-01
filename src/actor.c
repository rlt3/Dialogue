#include <pthread.h>
#include "actor.h"
#include "script.h"
#include "utils.h"

struct Actor {
    lua_State *L;
    Script *script;
    pthread_mutex_t mutex;
};

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index)
{
    return (Actor *) luaL_checkudata(L, index, ACTOR_LIB);
}

static int
lua_actor_new (lua_State *L)
{
    Actor *actor = lua_newuserdata(L, sizeof(Actor));
    luaL_getmetatable(L, ACTOR_LIB);
    lua_setmetatable(L, -2);

    actor->L = NULL;
    actor->script = NULL;
    actor->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    return 1;
}

static const luaL_Reg actor_methods[] = {
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    return lua_meta_open(L, ACTOR_LIB, actor_methods, lua_actor_new);
}
