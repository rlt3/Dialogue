#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define SCRIPT_LIB "Dialogue.Actor.Script"

struct Script;
typedef struct Script Script;

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index);

/*
 * Push the object of a Script at index.
 */
void
script_push_object (lua_State *L, int index);

/*
 * Push a table of a Script at index.
 */
void
script_push_table (lua_State *L, int index);

/*
 * Expects a Script table at index. Return the module name from the table.
 * Doesn't push to the stack.
 */
const char *
script_module_string (lua_State *L, int index);

/*
 * Get the module's name and load it. Doesn't push to the stack.
 */
void
script_module_load (lua_State *L, int index);

int 
luaopen_Script (lua_State *L);

#endif
