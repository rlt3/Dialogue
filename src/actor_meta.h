#ifndef DIALOGUE_ACTOR_META
#define DIALOGUE_ACTOR_META

#define ACTOR_META "Dialogue.Company.Actor"

/*
 * The Actors themselves are created by the Company (from the company_meta.c).
 * But their references we push as a table with the actor's id and the company
 * pointer and attach the actor metamethods to it.
 */

#include "dialogue.h"
#include "company.h"

/*
 * Push an Actor reference object onto the Lua stack.
 */
void
lua_push_actor (lua_State *L, int actor_id, struct Company *company);

int
luaopen_Dialogue_Company_Actor (lua_State *L);

#endif
