#include "action.h"
#include "actor.h"

/* All workers have the dialogue_table at the bottom of the stack */
static const int dialogue_table = 1;

/*
 * Director.create(definition_table [, parent])
 *
 * Director{"create", { {"draw", 2, 2} }, parent}
 * Director{"create", { {"draw", 2, 2} } }
 */
int
lua_action_create (lua_State *L)
{
    Director *director;
    Actor *actor, *parent = NULL;
    const int definition_arg = 1;
    const int parent_arg = 2;
    const int args = lua_gettop(L);

    if (args == parent_arg)
        parent = lua_touserdata(L, parent_arg);

    /* TODO: should this action *know* about __ptr? */
    lua_getfield(L, dialogue_table, "Director");
    lua_getfield(L, -1, "__ptr");
    director = lua_touserdata(L, -1);
    lua_pop(L, 2);

    /*
     * TODO:
     *      Push the actor pointer to a table (array) which is a collection of
     * actor references so that when we cleanup the Worker states, we can clear
     * out all created Actors.
     *      This means for deleting actors while the system is running requires
     * going back to the Worker that created it, which means we need each Actor
     * to store the Worker as a reference.
     *      Would it be easier to have each action as a C function instead of
     * a Lua function? Maybe we pass the Worker (which has the director pointer
     * in its struct to stop the above nonsense) as a first argument (like a 
     * method call)?
     */

    actor = actor_create(L, director, parent);

    /*
     * TODO:
     *      Workers should have Dialogue.Director in its state and Actors 
     * should have Dialogue.Actor. We make sure all states have the same 
     * structure (Dialogue table *then* the submodule) so we can have 
     * uniformity. E.g. the above delcaration doesn't work in the main thread
     * (the Director's thread) because the main thread has its declaration like
     * Dialogue.Director.
     */

    /* Call Director{ "load", actor } so scripts load asynchronously */
    lua_getfield(L, dialogue_table, "Director");

    lua_newtable(L);

    lua_pushliteral(L, "load");
    lua_rawseti(L, -2, 1);

    lua_pushlightuserdata(L, actor);
    lua_rawseti(L, -2, 2);

    lua_call(L, 1, 0);

    return 0;
}

int
lua_action_bench (lua_State *L)
{
    //const int actor_arg = 1;
    //const char *actor = luaL_checkstring(L, actor_arg);
    //printf("Benching %s\n", actor);
    return 0;
}

int
lua_action_join (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Joining %s\n", actor);
    return 0;
}

int
lua_action_receive (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Sending %s\n", actor);
    return 0;
}

int
lua_action_send (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Sending %s\n", actor);
    return 0;
}

int
lua_action_load (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Loading %s\n", actor);
    return 0;
}

int
lua_action_error (lua_State *L)
{
    const int error_arg = 1;
    const char *error = luaL_checkstring(L, error_arg);
    printf("%s\n", error);
    /*
     * TODO
     *    Use a statically defined mutex to log error to a specific lua table.
     *    then print it out.
     *    we can use lua_pushfstring to define error messages.
     */
    return 0;
}
