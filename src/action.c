#include "action.h"
#include "director.h"
#include "worker.h"
#include "actor.h"

/*
 * Since all Actions are methods of the Director metatable, the first argument
 * of all Actions will be the Director reference. This means we do not have to
 * keep up with references to the global Dialogue table and that all methods
 * share a common definition of the Director table at index 1.
 */

static const int director_self = 1;

/*
 * Synchronous:
 * Director:new(definition_table [, parent])
 *
 * Asynchronous:
 * Director{"new", definition_table [, parent]}
 *
 * See Actor.h (specifically function `actor_create') for the definition of
 * `definition_table'.
 *
 * Create an Actor and then load it asynchronously.
 */
int
lua_action_create (lua_State *L)
{
    /* args: director, definition, [parent,] worker */
    Worker *worker;
    Director *director;
    Actor *actor, *parent = NULL;
    const int definition_arg = 2;
    const int parent_arg = 3;
    const int args = lua_gettop(L) - 1;

    luaL_checktype(L, director_self, LUA_TTABLE);
    luaL_checktype(L, definition_arg, LUA_TTABLE);

    /* the Worker is an optional argument passed last */
    worker = lua_touserdata(L, args + 1);
    lua_pop(L, 1);

    /* get & pop the optional parent arg to keep our stack top at 2 */
    if (args == parent_arg) {
        parent = lua_touserdata(L, parent_arg);
        lua_pop(L, 1);
    }

    /* TODO: should this action *know* about __ptr? */
    lua_getfield(L, director_self, "__ptr");
    director = lua_touserdata(L, -1);
    lua_pop(L, 1);

    actor = actor_create(L, director, parent);
    lua_pop(L, 1); /* definition table */

    /*
     * If instead of `worker_save_actor' there was a `director_save_actor', I
     * wouldn't need to rely on the Worker state to keep our list of Actors.
     * *AND* this lets us this method synchronously & asynchronously. The 
     * Worker is supposed to be an *optional* argument denoting which thread
     * was used to create an Actor. 
     *
     * If it doesn't exist, I want to be able to assume it was the main thread
     * and a synchronous action. This is because eventually the Director will
     * become (or create) its own (custom) Worker to work on the main thread.
     * So, asynchronous Actions, as I'm thinking, will *always* be called with
     * that optional argument.
     */
    worker_save_actor(L, actor);

    /* 
     * after popping, director_self happens to be at top of index.
     * Call Director{ "load", actor } so scripts load asynchronously
     */
    lua_newtable(L);

    lua_pushliteral(L, "load");
    lua_rawseti(L, -2, 1);

    lua_pushlightuserdata(L, actor);
    lua_rawseti(L, -2, 2);

    lua_call(L, 1, 0);

    lua_pushlightuserdata(L, actor);
    return 1;
}

int
lua_action_bench (lua_State *L)
{
    return 0;
}

int
lua_action_join (lua_State *L)
{
    return 0;
}

int
lua_action_receive (lua_State *L)
{
    return 0;
}

int
lua_action_send (lua_State *L)
{
    return 0;
}

int
lua_action_load (lua_State *L)
{
    const int actor_arg = 2;
    Actor *actor = lua_touserdata(L, actor_arg);
    printf("Loading %p\n", actor);
    return 0;
}

int
lua_action_error (lua_State *L)
{
    /*
     * TODO
     *    Use a statically defined mutex to log error to a specific lua table.
     *    then print it out.
     *    we can use lua_pushfstring to define error messages.
     */
    return 0;
}
