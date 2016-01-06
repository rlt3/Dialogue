#include "action.h"
#include "actor.h"
#include "script.h"
#include "utils.h"

int
action_create (lua_State *L)
{
    puts("Create.");
    return 0;
}

int
action_bench (lua_State *L)
{
    puts("Bench.");
    return 0;
}

int
action_join (lua_State *L)
{
    puts("Join.");
    return 0;
}

int
action_remove (lua_State *L)
{
    puts("Remove.");
    return 0;
}

int
action_delete (lua_State *L)
{
    puts("Delete.");
    return 0;
}

int
action_load (lua_State *L)
{
    Script *script;
    Actor *actor;

    /*
     * TODO:
     *  Copying Actors between States safely.
     */
    lua_rawgeti(L, -1, 1);
    actor = lua_touserdata(L, -1);
    lua_pop(L, 1);

    for (script = actor->script_head; script != NULL; script = script->next)
        if (script->be_loaded)
            script_load(script);

    return 0;
}

int
action_send (lua_State *L)
{
    lua_State *A;
    Script *script;
    Actor *actor;

    lua_rawgeti(L, -1, 1);
    actor = lua_touserdata(L, -1);
    lua_pop(L, 1);

    /* 
     * get the tail of the table from index 3 and greater because that's the
     * actor's envelope form
     */
    utils_push_table_sub(L, lua_gettop(L), 3);

    A = actor_request_state(actor);
    utils_copy_top(A, L);

    /*
     * TODO:
     *      Error handling for different return types for script_send.
     */
    for (script = actor->script_head; script != NULL; script = script->next)
        if (script->is_loaded)
              if (script_send(script) == SEND_FAIL)
                  printf("%s\n", script->error);

    lua_pop(A, 1);
    actor_return_state(actor);

    lua_pop(L, 1);
    return 0;
}

int
action_error (lua_State *L)
{
    puts("Error.");
    return 0;
}
