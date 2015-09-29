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

int 
luaopen_Envelope (lua_State *L);

#endif
