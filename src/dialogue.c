#include "dialogue.h"
#include "director.h"
#include "action.h"
#include "worker.h"

static const luaL_Reg dialogue_actions[] = {
    {"new",     lua_action_create},
    {"bench",   lua_action_bench},
    {"join",    lua_action_join},
    {"receive", lua_action_receive},
    {"send",    lua_action_send},
    {"load",    lua_action_load},
    {"error",   lua_action_error},
    { NULL, NULL }
};

static const luaL_Reg director_metamethods[] = {
    {"__call",     lua_director_action},
    {"__tostring", lua_director_tostring},
    { NULL, NULL }
};

static inline void
create_dialogue_table (lua_State *L)
{
    lua_newtable(L);
    luaL_setfuncs(L, dialogue_actions, 0);

    luaL_newmetatable(L, "Dialogue");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, director_metamethods, 0);

    lua_setmetatable(L, -2);
}

/*
 * The `require' function. Create the Dialogue table in the global Lua state
 * and add the garbage collection function to the metatable.
 */
int
luaopen_Dialogue (lua_State *L)
{
    create_dialogue_table(L);
    lua_getfield(L, -1, "__index");
    lua_pushcfunction(L, lua_director_quit);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);
    return 1;
}
