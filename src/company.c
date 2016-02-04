#include <stdlib.h>
#include "tree.h"
#include "actor.h"
#include "company.h"

#define COMPANY_META "Dialogue.Company"
#define ACTOR_META "Dialogue.Company.Actor"

/*
 * Create the Company tree with the following options.
 */
int
company_create (int base_actors, int max_actors, int base_children)
{
    return tree_init(base_actors, max_actors, 2, 
            actor_assign_id, actor_destroy, actor_get_id);
}

int 
company_add (lua_State *L, int parent, int thread_id)
{
    return tree_add_reference(actor_create(L, thread_id), parent);
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
 * An actor can be represented in many ways. All of them boil down to an id.
 * This function coerces many actor-types into getting the id it holds.
 */
int
company_actor_id (lua_State *L, int index)
{
    int id, type = lua_type(L, index);

    switch(type) {
    case LUA_TTABLE:
        lua_rawgeti(L, index, 1);
        id = lua_tointeger(L, -1);
        lua_pop(L, 1);
        break;

    case LUA_TNUMBER:
        id = lua_tointeger(L, index);
        break;
        
    case LUA_TSTRING:
        break;

    default:
        luaL_error(L, "Cannot get id from unexpected Actor type");
        break;
    }

    return id;
}

/*
 * -- find the actor that represents "dungeon" and return an actor object
 * room = Actor("dungeon")
 *
 * -- create an actor. its parent is the dungeon room
 * Actor({"entity", "player.png"}, room)
 *
 * -- create an actor with the name "graphics". lookup parent by name and
 * -- limit the operations of this actor to the main thread (thread 0).
 * Actor("graphics", {"window", 400, 600}, "dungeon", 0)
 *
 * So, an Actor can be represented in a multitude of ways:
 *   - id: all ways boil down to this -- all actors are an id
 *   - object: actor objects are just tables that hold an id
 *   - string: string name that gets mapped to an id
 */
int
lua_company_call (lua_State *L)
{
    return 0;
}

/*
 * Actor( definition_table [, parent] [, thread] )
 *
 * Creates the actor from the given definition table (see actor.h for more
 * info). An optional Actor object that should be the parent of the created 
 * Actor can be passed.
 */
int
lua_actor_new (lua_State *L)
{
    const int definition_arg = 2;
    const int parent_arg = 3;
    const int thread_arg = 4;
    int args = lua_gettop(L);
    int parent = NODE_INVALID;
    int thread = NODE_INVALID;
    int id;
    
    luaL_checktype(L, definition_arg, LUA_TTABLE);

    /* get the thread id, then pop it and any extra args */
    if (args >= thread_arg) {
        thread = luaL_checkinteger(L, thread_arg);
        lua_pop(L, args - thread_arg + 1);
        args = parent_arg;
    }

    /* get the parent id, then pop it and extra args, leaving table on top */
    if (args >= parent_arg) {
        parent = company_actor_id(L, parent_arg);
        lua_pop(L, args - parent_arg + 1);
    }

    id = company_add(L, parent, thread);

    switch (id) {
    case ERROR:
        luaL_error(L, "Creating Actor failed!");
        break;

    case NODE_ERROR:
        luaL_error(L, "Invalid parent `%d` for new actor!", parent);
        break;

    case NODE_INVALID:
        luaL_error(L, "Unable to allocate memory for new Actor!");
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

/*
 * actor:child( definition_table )
 * Call lua_actor_new with the actor (owner of the child method) as the parent.
 */
int
lua_actor_child (lua_State *L)
{
    /* expects actor @ 1 and definition table @ 2 */
    const int expected_args = 2;
    const int args = lua_gettop(L);

    if (args != expected_args)
        luaL_error(L, "Invalid # of args to actor:child. Expected %d got %d\n",
                expected_args, args);

    /* shift the arguments around: actor @ 3, table @ 2, nothing at bottom */
    lua_pushnil(L);
    lua_insert(L, 1);
    lua_insert(L, 2);
    lua_actor_new(L);

    return 1;
}

int
lua_actor_children (lua_State *L)
{
    /*
     * TODO: tree_map_subtree for functions with (void *, int) that can map to
     * (lua_State*, int id) functions?
     *
     * It would enable: tree_map_subtree(id, L, company_push_actor, no_recurse)
     */
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

    if (company_remove(id) != 0)
        luaL_error(L, "Cannot delete invalid reference `%d`!", id);
    
    return 0;
}

/*
 * actor:lock()
 *
 * Locks the specific actor. This will cause all threads that *need* (and can't
 * skip) the reference to block and wait. This will also cause memory leaks if
 * the actor isn't unlocked before the system quits.
 *
 * Returns true/false if the actor locked or not.
 */
int
lua_actor_lock (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    lua_pushboolean(L, (company_ref(id) != NULL));
    return 1;
}

/*
 * actor:unlock()
 *
 * Unlocks a previously locked actor. Unlocking a not-locked actor is undefined.
 * Returns true/false if the actor was unlocked or not.
 */
int
lua_actor_unlock (lua_State *L)
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
    {"lock",     lua_actor_lock},
    {"unlock",   lua_actor_unlock},
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
