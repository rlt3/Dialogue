#ifndef DIALOGUE_COMPANY_META
#define DIALOGUE_COMPANY_META

/*
 * Since all Lua metatables should point to the same Actor, we have the ability
 * for many states to create Actors in one logical spot -- the Company.
 *
 * The Company Lua metatable is a wrapper for creating Actors inside Lua. 
 *
 * We call the Company inside lua "Actor" to avoid confusion.
 */

#define COMPANY_GLOBAL "Actor"
#define COMPANY_META "Dialogue.Company"

#include "dialogue.h"
#include "company.h"
#include "actor.h"

/*
 * Expects a table on top of the Lua stack. Set the company in that table.
 */
void
company_set_table (lua_State *L, Company *company);

/*
 * Push an Actor reference object onto the Lua stack.
 */
void
company_push_actor (lua_State *L, int actor_id, Company *company);

/*
 * Create a Company meta table inside the given Lua state.
 */
void
company_open (lua_State *L, Company *company);

#endif
