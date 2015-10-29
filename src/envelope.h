#ifndef DIALOGUE_ENVELOPE
#define DIALOGUE_ENVELOPE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define ENVELOPE_LIB "Dialogue.Mailbox.Envelope"

struct Mailbox;
struct Envelope;
struct Actor;

/*
 * The Envelope holds the metadata of a message. It exists inside a Mailbox.
 * This provides an easy way to transport messages between Actors as well as
 * between the interpreter and the Mailbox and then to Actors.
 */
typedef struct Envelope {
    struct Actor *author;
    struct Actor *recipient;
    struct Mailbox *mailbox;
    const char *tone;
    int message_ref;
} Envelope;

/*
 * Check for a Envelope at index. Errors if it isn't an Envelope.
 */
Envelope *
lua_check_envelope (lua_State *L, int index);

int 
luaopen_Dialogue_Envelope (lua_State *L);

#endif
