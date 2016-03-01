#ifndef DIALOGUE_DIRECTOR
#define DIALOGUE_DIRECTOR

#include "dialogue.h"

/*
 * Load the Director and all of the Workers.
 */
int
director_create ();

/*
 * Set the Director inside the Lua state.
 */
void
director_set (lua_State *L);

/*
 * Expects an Action on top of the Lua stack in the form:
 *     { action, actor [, data1 [, ... [, dataN]]] }
 *
 * { OP_BENCH, bullet }
 * { OP_SEND, player, "input", "JOYSTICK-FIRE" }
 * { OP_LOAD, bullet, "location", x, y }
 * { OP_JOIN, bullet }
 * { OP_SEND, bullet, "target", 30, 40 }
 *
 * Send the action to an open worker. Returns 0 if successful.
 */
int
director_action (lua_State *L);

/*
 * Close the Director and all of the Workers.
 */
void
director_close ();

#endif
