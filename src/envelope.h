#ifndef DIALOGUE_ENVELOPE
#define DIALOGUE_ENVELOPE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define ENVELOPE_LIB "Dialogue.Envelope"

struct Envelope;
struct Actor;
typedef void (*Tone) (struct Envelope*);

/*
 * The Envelope is what holds an individual message in a stream while the 
 * system is processing other messages. It is the C representation of a
 * table for us to easily pass around.
 */

typedef struct Envelope {
    struct Envelope *next;
    struct Actor *author;
    struct Actor *recipient;
    Tone tone;
    const char **data;
    int data_len;
} Envelope;

/*
 * Check for a Envelope at index. Errors if it isn't an Envelope.
 */
Envelope *
lua_check_envelope (lua_State *L, int index);

/*
 * Create an envelope that will fail a bind call.
 */
Envelope
envelope_create_empty();

/*
 * Determine if an envelope should be called like f(envelope) or not.
 */
void
envelope_bind(Envelope, void (*f) (Envelope));

/*
 * Push the table of an Envelope onto given lua_State.
 */
void
envelope_push_table (lua_State *L, Envelope *envelope);

void
envelope_free (Envelope *envelope);

int 
luaopen_Dialogue_Envelope (lua_State *L);

#endif
