#include <stdlib.h>
#include "tree.h"
#include "company.h"

#define COMPANY_META "Dialogue.Company"
#define ACTOR_META "Dialogue.Company.Actor"

/*
 * Make, Lookup, Set Id, and Remove. Allocate some data and free it. Used to
 * test we don't have memory leaks.
 */

void* mk (int id) {
        return malloc(sizeof(int));
}

void set (void *data, int id) {
        *((int*)data) = id;
}

int lk (void *data) {
        return *((int*)data);
}

void rm (void *data) {
        free(data);
}

/*
 * Create the Company tree with the following options.
 */
int
company_create (int base_actors, int max_actors, int base_children)
{
    return tree_init(base_actors, max_actors, 2, set, rm, lk);
}

int 
company_add (int parent)
{
    return tree_add_reference(mk(0), parent);
}

int
company_remove (int id)
{
    return tree_unlink_reference(id, 1);
}

void *
company_ref (int id)
{
    return tree_ref(id);
}

int
company_deref (int id)
{
    return tree_deref(id);
}

void
company_close ()
{
    tree_cleanup();
}

/*
 * Push an Actor reference object onto the Lua stack.
 */
void
company_push_actor (lua_State *L, int actor_id)
{
    lua_newtable(L);

    lua_pushinteger(L, actor_id);
    lua_rawseti(L, -2, 1);

    luaL_getmetatable(L, ACTOR_META);
    lua_setmetatable(L, -2);
}

/*
 * Get the Actor's id from the actor Lua reference at index.
 */
int
company_actor_id (lua_State *L, int index)
{
    int id;
    luaL_checktype(L, index, LUA_TTABLE);
    lua_rawgeti(L, index, 1);
    id = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return id;
}

int
lua_actor_new (lua_State *L)
{
    const int parent_arg = 2;
    const int args = lua_gettop(L);
    int parent, id;

    if (args == parent_arg)
        parent = company_actor_id(L, parent_arg);
    else
        parent = NODE_INVALID;

    id = company_add(parent);

    switch (id) {
    case ERROR:
        luaL_error(L, "NULL data given to Company");
        break;

    case NODE_ERROR:
        luaL_error(L, "Invalid parent for new Actor");
        break;

    case NODE_INVALID:
        luaL_error(L, "Unable to allocate memory for new Actor");
        break;

    default:
        company_push_actor(L, id);
        break;
    }

    return 1;
}

int
lua_actor_load (lua_State *L)
{
    return 0;
}

int
lua_actor_child (lua_State *L)
{
    return 0;
}

int
lua_actor_children (lua_State *L)
{
    return 0;
}

int
lua_actor_bench (lua_State *L)
{
    return 0;
}

int
lua_actor_delete (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    lua_pushboolean(L, (company_remove(id) == 0));
    return 1;
}

int
lua_actor_ref (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    lua_pushboolean(L, (company_ref(id) != NULL));
    return 1;
}

int
lua_actor_deref (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    lua_pushboolean(L, (company_deref(id) == 0));
    return 1;
}

int
lua_actor_id (lua_State *L)
{
    const int actor_arg = 1;
    lua_pushinteger(L, company_actor_id(L, actor_arg));
    return 1;
}

int
lua_actor_probe (lua_State *L)
{
    return 0;
}

static const luaL_Reg actor_metamethods[] = {
    {"load",     lua_actor_load},
    {"child",    lua_actor_child},
    {"children", lua_actor_children},
    {"delete",   lua_actor_delete},
    {"ref",      lua_actor_ref},
    {"deref",    lua_actor_deref},
    {"id",       lua_actor_id},
    {"bench",    lua_actor_bench},
    {"probe",    lua_actor_probe},
    { NULL, NULL }
};

static const luaL_Reg company_metamethods[] = {
    {"__call",   lua_actor_new},
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

void
company_set (lua_State *L)
{
    luaL_requiref(L, "Actor", luaopen_Dialogue_Company, 1);
    lua_pop(L, 1);
}
