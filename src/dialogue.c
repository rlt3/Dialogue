#include "dialogue.h"
#include "director.h"
#include "action.h"
#include "worker.h"

static const luaL_Reg dialogue_actions[] = {
    {"new",   lua_action_create},
    {"bench", lua_action_bench},
    {"join",  lua_action_join},
    {"send",  lua_action_send},
    {"load",  lua_action_load},
    {"error", lua_action_error},
    { NULL, NULL }
};

static const luaL_Reg director_metamethods[] = {
    {"__call",     lua_director_action},
    {"__gc",       lua_director_quit},
    {"__tostring", lua_director_tostring},
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
