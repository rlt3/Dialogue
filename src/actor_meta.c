#include "actor_meta.h"

/*
 *  Dialogue.Actor({ ["Star"|"Lead",] [ { "module" [, arg1 [, ... [, argN]]]} ] } [, Parent])
 *  Create an Actor. Optionally, create an Actor as a child of Parent.
 */
int
lua_actor_new (lua_State *L);

/*
 * actor:child{ ["Star"|"Lead",] [ { "module" [, arg1 [, ... [, argN]]]} ] }
 * Create a child of the `actor' object.
 */
int
lua_actor_child (lua_State *L);

/*
 * actor:children()
 * Return an array of an Actor's children.
 */
int
lua_actor_children (lua_State *L);

/*
 * actor:load([script_index])
 * Load any particular script, or all scripts of an Actor.
 */
int
lua_actor_load (lua_State *L);

/*
 * actor:probe(script_index, field)
 * Probe an Actor's script object (at index) for the given field.
 */
int
lua_actor_load (lua_State *L);

/*
 * Push an Actor to the given Lua state as lightuserdata and attach the
 * metatable to it.
 */
void
lua_push_actor (lua_State *L, Actor *actor);

int 
luaopen_Dialogue_Actor (lua_State *L)
{
    lua_newtable(L);

    luaL_newmetatable(L, ACTOR_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    lua_setmetatable(L, -2);

    return 1;
}
