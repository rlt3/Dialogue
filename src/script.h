#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define SCRIPT_LIB "Dialogue.Actor.Script"

typedef struct Script {
    int table_reference;
    int object_reference;
    int is_loaded;
    int ref;
    struct Script *next;
    struct Actor *actor;
} Script;

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index);

/*
 * Push the object onto the Actor's state. We pass along the calling state
 * for error handling.
 */
void
script_push_object (Script *script, lua_State *L);

/*
 * Push a table of a Script at index.
 */
void
script_push_table (lua_State *L, Script *script);

int 
luaopen_Dialogue_Actor_Script (lua_State *L);

#endif
