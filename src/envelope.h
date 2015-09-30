#ifndef DIALOGUE_ENVELOPE
#define DIALOGUE_ENVELOPE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define ENVELOPE_LIB "Dialogue.Envelope"

/*
 * The Envelope is a reference to a table (array) where the first element is
 * the title of a message (the method name) and is required. Anything after the
 * first is data (arguments to a method).
 */

struct Envelope;
typedef struct Envelope Envelope;

/*
 * Checks for an Envelope at index.
 * Push an envelope's table onto the stack.
 */
void
envelope_push_table (lua_State *L, int index);

/*
 * Push the title of an envelope at index onto the top of the stack.
 */
void
envelope_push_title (lua_State *L, int index);

/*
 * Push the envelope (at index) data sequentially onto the stack.
 * Returns the number of args pushed.
 */
int
envelope_push_data (lua_State *L, int index);

int 
luaopen_Envelope (lua_State *L);

#endif
