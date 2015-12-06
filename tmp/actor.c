#include "actor.h"
#include "utils.h"

/*
 * Check for an Actor at index. Errors if it isn't an Actor.
 */
Actor *
lua_check_actor (lua_State *L, int index)
{
    return (Actor *) luaL_checkudata(L, index, ACTOR_LIB);
}

/*
 * Create an Actor which has its own thread. It initializes all Scripts and 
 * handles all messages (send/receive) in its own thread because many Lua 
 * modules and objects aren't thread-safe.
 *
 * Create the Actor by sending a table of tables of the Lua module to load and
 * any variables needed to initialize it.
 *
 * Actor{ {"draw", 400, 200}, {"weapon", "longsword"} };
 */
static int
lua_actor_new (lua_State *L)
{
    Actor *actor;
    lua_State *A;
    pthread_mutexattr_t mutex_attr;

    luaL_checktype(L, 1, LUA_TTABLE);

    actor = lua_newuserdata(L, sizeof(Actor));
    luaL_getmetatable(L, ACTOR_LIB);
    lua_setmetatable(L, -2);

    actor->parent = NULL;
    actor->next = NULL;
    actor->child = NULL;
    actor->script = NULL;
    actor->mailbox = NULL;
    actor->dialogue = NULL;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&actor->stack_mutex, &mutex_attr);
    pthread_mutex_init(&actor->state_mutex, &mutex_attr);

    actor->L = luaL_newstate();
    A = actor->L;

    luaL_openlibs(A);

    /* load this module (the one you're reading) into the Actor's state */
    //luaL_requiref(A, "Dialogue", luaopen_Dialogue, 1);
    //lua_pop(A, 1);

    /* push Actor so Scripts can reference the Actor it belongs to. */
    utils_push_object(A, actor, ACTOR_LIB);
    lua_setglobal(A, "actor");

    /* copy and set table which defines the actor to a global for easy access */
    utils_copy_top(A, L);
    lua_setglobal(A, "__load_table");

    /* make a table for envelopes to sit in */
    lua_newtable(A);
    lua_setglobal(A, "__envelopes");

    actor->new_action = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    actor->action = LOAD;
    actor->on = 1;

    pthread_create(&actor->thread, NULL, actor_thread, actor);
    pthread_detach(actor->thread);

    return 1;
}

/*
 * Set the thread's condition to false and close the Lua stack.
 */
static int
lua_actor_gc (lua_State *L)
{
    lua_State *A;
    Actor *actor = lua_check_actor(L, 1);

    actor_request_state(actor);
    actor->on = 0;
    actor_return_state(actor);

    A = actor_request_stack(actor);
    lua_close(A);
    actor_return_stack(actor);

    return 0;
}

static const luaL_Reg actor_methods[] = {
    {"__gc",       lua_actor_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    return utils_lua_meta_open(L, ACTOR_LIB, actor_methods, lua_actor_new);
}
