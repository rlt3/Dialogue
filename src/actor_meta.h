#ifndef DIALOGUE_ACTOR_META
#define DIALOGUE_ACTOR_META

/*
 * This file describes the Actor's metatable inside Lua. Since Actors cannot be
 * Lua userdata (since an Actor can be processed by many Workers with their own
 * Lua states), we rely on pushing an Actor pointer as lightuserdata and 
 * attaching the Actor metatable to it.
 *
 * Because all lightuserdata share the same metatable, we make sure to pass the 
 * Actor as a pointer between Workers and use the pointer in an Actor's own
 * Lua state.
 *
 * We only use lua_push_actor for the interpreter (or main) Lua state.
 *
 * TODO:
 *      Why can't we call lua_newuserdata for an Actor inside an Actor's lua_State?
 *  It is mind-bending, but it would solve the problem of not polluting the 
 *  lightuserdata metatable user-space.
 *
 *  lua_State *L = new_luastate()
 *
 *  Actor *actor = newuserdata(L);
 *  actor->L = L;
 *
 *  why can't this work?
 */

#define ACTOR_META "Dialogue.Actor"
#include "dialogue.h"
#include "actor.h"

/*
 * Push an Actor to the given Lua state as lightuserdata and attach the
 * metatable to it.
 */
void
lua_push_actor (lua_State *L, Actor *actor);

int 
luaopen_Dialogue_Actor (lua_State *L);

#endif
