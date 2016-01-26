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
 * Return the id of the actor object reference at the index.
 */
static inline int
lua_actor_id (lua_State *L, int index)
{
    int id;
    lua_rawgeti(L, index, 1);
    id = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return id;
}

/*
 * Get the Company of the actor object reference at the index.
 */
static inline Company *
lua_actor_company (lua_State *L, int index)
{
    Company *company;
    lua_rawgeti(L, index, 2);
    company = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return company;
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
 * actor:delete()
 * Permanently delete the Actor from the system.
 */
int
lua_actor_delete (lua_State *L)
{
    const int actor_arg = 1;
    const int id = lua_actor_id(L, actor_arg);
    Company *company = lua_actor_company(L, actor_arg);
    printf("Company %p\n", company);
    //company_remove_actor(company, id);
    return 0;
}

/*
 * actor:bench()
 * Temporarily removes the Actor from the system. This method implies you will
 * call actor:join() later to reattach the actor to the system.
 */
int
lua_actor_bench (lua_State *L)
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
    {"delete",   lua_actor_delete},
    {"bench",    lua_actor_bench},
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
