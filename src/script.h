#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define SCRIPT_LIB "Dialogue.Actor.Script"
#define SEND_OK   0
#define SEND_SKIP 1
#define SEND_FAIL 2

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
 * Push the object onto the Actor's state.
 */
void
script_push_object (Script *script);

/*
 * Send the message at the given index. Returns SEND_OK if the message was sent
 * OK, SEND_SKIP if there was no function matching the message's first element,
 * and SEND_FAIL if there was a function and an error occurred.
 */
int
script_send_message (Script *script, int message_table);

/*
 * Push a table of a Script at index.
 */
void
script_push_table (lua_State *L, Script *script);

int 
luaopen_Dialogue_Actor_Script (lua_State *L);

#endif
