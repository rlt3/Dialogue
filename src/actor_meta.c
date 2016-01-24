#include "actor_meta.h"
#include "company_meta.h"

/*
 * Push an Actor reference object onto the Lua stack.
 */
void
lua_push_actor (lua_State *L, int actor_id, Company *company)
{
    lua_newtable(L);

    lua_pushinteger(L, actor_id);
    lua_rawseti(L, -2, 1);

    company_set_table(L, company);

    luaL_getmetatable(L, ACTOR_META);
    lua_setmetatable(L, -2);
}

/*
 * actor:child{ ["Star"|"Lead",] [ { "module" [, arg1 [, ... [, argN]]]} ] }
 * Create a child of the `actor' object.
 */
int
lua_actor_child (lua_State *L)
{
    return 1;
}

/*
 * actor:children()
 * Return an array of an Actor's children.
 */
int
lua_actor_children (lua_State *L)
{
    return 1;
}

/*
 * actor:load([script_index])
 * Load any particular script, or all scripts of an Actor.
 */
int
lua_actor_load (lua_State *L)
{
    return 1;
}

/*
 * actor:probe(script_index, field)
 * Probe an Actor's script object (at index) for the given field.
 */
int
lua_actor_probe (lua_State *L)
{
    return 1;
}

static const luaL_Reg actor_metamethods[] = {
    {"load",     lua_actor_load},
    {"child",    lua_actor_child},
    {"children", lua_actor_children},
    {"probe",    lua_actor_probe},
    { NULL, NULL }
};

int
luaopen_Dialogue_Company_Actor (lua_State *L)
{
    luaL_newmetatable(L, ACTOR_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, actor_metamethods, 0);
    return 0;
}
