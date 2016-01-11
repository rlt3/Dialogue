#ifndef DIALOGUE_DIRECTOR
#define DIALOGUE_DIRECTOR

#include "dialogue.h"

typedef struct Director Director;

/*
 * Set the Director to a table at the index.
 */
void
director_set (lua_State *L, int index, Director *director);

/*
 * Receive an action in this form:
 *     Dialogue{ action, actor [, data1 [, ... [, dataN]]] }
 *
 * Send the action to an open worker.
 */
int
lua_director_action (lua_State *L);

/*
 * Stop any threads and free any memory associated with them and the director.
 */
int
lua_director_quit (lua_State *L);

int
lua_director_tostring (lua_State *L);

/*
 * Create the Director table with all the Actions in the given Lua state.
 */
void
create_director_table (lua_State *L);

/*
 * Create the Director table using above, but add the garbage collection
 * function to the metatable.
 */
int
luaopen_Dialogue_Director (lua_State *L);

#endif
