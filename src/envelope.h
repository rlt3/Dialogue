#ifndef DIALOGUE_ENVELOPE
#define DIALOGUE_ENVELOPE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define ENVELOPE_LIB "Dialogue.Post.Envelope"

/*
 * The Envelope is the core piece of data for Dialogue. 
 *
 * The Envelope tells a Postman what to do next with an Actor. That action may
 * be to send a message. Or it might be to create a child. Or to reload. Since
 * all actions happen through an Actor, all messages will have an Actor author.
 *
 * The form of an Envelope is (author, action, data).
 */

struct Mailbox;
struct Actor;

typedef struct Envelope {
    struct Actor *author;
    Action action;
    int message_ref;
} Envelope;

int 
luaopen_Dialogue_Actor_Mailbox_Envelope (lua_State *L);

#endif
