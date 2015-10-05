#ifndef DIALOGUE_ENVELOPE
#define DIALOGUE_ENVELOPE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define ENVELOPE_LIB "Dialogue.Envelope"

typedef struct Envelope {
    int table_reference;
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

/*
 * Expects an Envelope table at index. Push the title of an envelope.
 */
void
envelope_push_title (lua_State *L, int index);

/*
 * Expects an Envelope table at index. Pushes all data onto the stack. Returns 
 * the number of args pushed.
 */
int
envelope_push_data (lua_State *L, int index);

int 
luaopen_Dialogue_Envelope (lua_State *L);

#endif
