#ifndef DIALOGUE_ENVELOPE
#define DIALOGUE_ENVELOPE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define ENVELOPE_LIB "Dialogue.Actor.Mailbox.Envelope"

struct Mailbox;
struct Actor;

typedef struct Envelope {
    struct Actor *author;
    int message_ref;
} Envelope;

/*
 * Check for a Envelope at index. Errors if it isn't an Envelope.
 */
Envelope *
lua_check_envelope (lua_State *L, int index);

int 
luaopen_Dialogue_Actor_Mailbox_Envelope (lua_State *L);

#endif
