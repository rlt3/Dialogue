#include "dialogue.h"
#include "director.h"
#include "worker.h"

int
lua_dialogue_create (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Creating %s\n", actor);
    return 0;
}

int
lua_dialogue_bench (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Benching %s\n", actor);
    return 0;
}

int
lua_dialogue_join (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Joining %s\n", actor);
    return 0;
}

int
lua_dialogue_send (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Sending %s\n", actor);
    return 0;
}

int
lua_dialogue_load (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Loading %s\n", actor);
    return 0;
}

int
lua_dialogue_error (lua_State *L)
{
    const int error_arg = 1;
    const char *error = luaL_checkstring(L, error_arg);
    printf("%s\n", error);
    return 0;
}

static const luaL_Reg dialogue_actions[] = {
    {"new",   lua_dialogue_create},
    {"bench", lua_dialogue_bench},
    {"join",  lua_dialogue_join},
    {"send",  lua_dialogue_send},
    {"load",  lua_dialogue_load},
    {"error", lua_dialogue_error},
    { NULL, NULL }
};

int
luaopen_Dialogue (lua_State *L)
{
    lua_newtable(L);

    luaL_newmetatable(L, "Dialogue");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, director_metamethods, 0);

    lua_setmetatable(L, -2);
    luaL_setfuncs(L, dialogue_actions, 0);

    return 1;
}
