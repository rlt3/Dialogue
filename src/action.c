#include "action.h"

int
lua_action_create (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Creating %s\n", actor);
    return 0;
}

int
lua_action_bench (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Benching %s\n", actor);
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
    return 0;
}
