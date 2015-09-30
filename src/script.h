#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define SCRIPT_LIB "Dialogue.Actor.Script"

/*
 * A Script is loaded into an Actor's lua_State as a table to be referenced
 * later. Each C-Script type holds a reference to that table and then also to
 * the next Script. A table allows us to have variable-length construct list.
 *
 * table = {
 *      construct = { 'baz', 20, 'start-it' },
 *      module = require 'foo',
 *      object = module.new(construct)
 * }
 */

struct Script;
typedef struct Script Script;

/*
 * Checks for a Script at index.
 * Push the script's object onto the stack.
 */
void
script_push_object (lua_State *L, int index);

int 
luaopen_Script (lua_State *L);

#endif
