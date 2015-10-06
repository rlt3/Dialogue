#ifndef DIALOGUE_ENVELOPE
#define DIALOGUE_ENVELOPE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define ENVELOPE_LIB "Dialogue.Envelope"

/*
 * The Envelope is what holds an individual message in a stream while the 
 * system is processing other messages. It is the C representation of a
 * table for us to easily pass around.
 */

typedef struct Envelope {
    int table_reference;
    struct Envelope *next;
    lua_State *L;
} Envelope;

/*
 * Check for a Envelope at index. Errors if it isn't an Envelope.
 */
Envelope *
lua_check_envelope (lua_State *L, int index);

/*
 * Push the table of an Envelope at index.
 */
void
envelope_push_table (lua_State *L, int index);

int 
luaopen_Dialogue_Envelope (lua_State *L);

#endif
