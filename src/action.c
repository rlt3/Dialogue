#include "action.h"

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
    puts("Load.");
    return 0;
}

int
action_send (lua_State *L)
{
    //puts("Send.");
    return 0;
}

int
action_error (lua_State *L)
{
    puts("Error.");
    return 0;
}
