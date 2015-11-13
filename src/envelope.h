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
 * The Envelope holds the metadata of a message. The message is simply a table.
 * In the real-world an Envelope holds the address of the recipient, but ours
 * is a tad different -- we hold the method for getting the recipients of the
 * message since each message could potentially have more than one recipient.
 *
 * In exceptional cases a recipient can be set explicity for a one-to-one
 * relay.
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
