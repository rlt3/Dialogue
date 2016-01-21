#include <stdlib.h>
#include "company_meta.h"

Company *
lua_get_company (lua_State *L, int index)
{
    Company *company;
    lua_getfield(L, index, "company");

    if (lua_type(L, -1) == LUA_TNIL)
        luaL_error(L, "Cannot get company: no Company pointer set!");

    company = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return company;
}

/*
 * Expects a table on top of the Lua stack. Set the company in that table.
 */
void
company_set_table (lua_State *L,  Company *company)
{
    lua_pushlightuserdata(L, company);
    lua_setfield(L, lua_gettop(L) - 1, "company");
}

/*
 * Push an Actor reference object onto the Lua stack.
 */
static inline void
lua_push_actor (lua_State *L, int actor_id, Company *company)
{
    /*
     * TODO:
     *  This function needs to increment the reference counter for actor
     *  because having an explicit actor object is a reference.
     *
     *  So, all of our functions will need to dereference the actor. and then
     *  reference it again if it sends a message. Since no actor can be used at
     *  once, and there is a mutex on its state, we can be absolutely sure that
     *  the reference count on an actor is.
     *
     *  If our system can handle bad references to an actor (by just throwing
     *  an error), why can't we just drop the actor? All the bad messages will
     *  be automatically 'cleaned up'. Because those references were just 
     *  integers, we don't have pointers hanging out there.
     */
    lua_newtable(L);

    lua_pushinteger(L, actor_id);
    lua_rawseti(L, -2, 1);

    company_set_table(L, company);

    luaL_getmetatable(L, ACTOR_META);
    lua_setmetatable(L, -2);
}

/*
 *  Actor({ ["Star"|"Lead",] [ { "module" [, arg1 [, ... [, argN]]]} ] } [, Parent])
 *  Create an Actor. Optionally, create an Actor as a child of Parent.
 */
int
lua_company_create (lua_State *L)
{
    const int meta_arg = 1;
    const int def_arg = 2;
    const int parent_arg = 3;
    int parent_id = -1;
    int actor_id;
    Company *company = lua_get_company(L, meta_arg);

    luaL_checktype(L, def_arg, LUA_TTABLE);

    if (lua_type(L, parent_arg) == LUA_TTABLE) {
        lua_rawgeti(L, parent_arg, 1);
        parent_id = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }

    actor_id = company_add_actor(company, L, parent_id);

    if (actor_id >= 0)
        lua_push_actor(L, actor_id, company);
    else
        luaL_error(L, "Bad Actor");
    /*
     * TODO:
     *    dialogue_error from a single location which operates off of an enum
     *    of errors.
     */

    return 1;
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

static const luaL_Reg company_metamethods[] = {
    {"__call", lua_company_create},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Company (lua_State *L)
{
    luaL_newmetatable(L, ACTOR_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, actor_metamethods, 0);

    lua_newtable(L);

    luaL_newmetatable(L, COMPANY_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, company_metamethods, 0);

    lua_setmetatable(L, -2);

    return 1;
}

/*
 * Create a Company meta table inside the given Lua state.
 */
void
company_open (lua_State *L, Company *company)
{
    luaL_requiref(L, "Actor", luaopen_Dialogue_Company, 1);
    company_set_table(L, company);
    lua_pop(L, 1);
}
