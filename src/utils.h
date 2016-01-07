#ifndef DIALOGUE_UTILS
#define DIALOGUE_UTILS

#include "dialogue.h"

/*
 * Expects a table at the top of the 'from' stack. Pushes table onto 'to' stack.
 */
void
utils_copy_table (lua_State *to, lua_State *from, int index);

/*
 * Copies the value at the top of 'from' to 'to'.
 */
void
utils_copy_top (lua_State *to, lua_State *from);

#endif
