#include <stdlib.h>
#include "company_meta.h"
#include "actor_meta.h"

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

static const luaL_Reg company_metamethods[] = {
    {"__call", lua_company_create},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Company (lua_State *L)
{
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
    luaL_requiref(L, ACTOR_META, luaopen_Dialogue_Company_Actor, 0);
    luaL_requiref(L, COMPANY_GLOBAL, luaopen_Dialogue_Company, 1);
    company_set_table(L, company);
    lua_pop(L, 1);
}
