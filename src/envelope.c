#include "envelope.h"
#include "actor.h"
#include "mailbox.h"
#include "post.h"
#include "utils.h"

/*
 * Check for a Envelope at index. Errors if it isn't an Envelope.
 */
Envelope *
lua_check_envelope (lua_State *L, int index)
{
    return (Envelope *) luaL_checkudata(L, index, ENVELOPE_LIB);
}

/*
 * Check the table at index for the requirements of an Envelope.
 */
void
envelope_check_table (lua_State *L, int index)
{
    int len;
    luaL_checktype(L, index, LUA_TTABLE);
    len = luaL_len(L, index);
    luaL_argcheck(L, len > 0, index, "Message needs to have a title!");
}

/*
 * Push the message from an Envelope at index.
 */
void
envelope_push_message (lua_State *L, int index)
{
    Envelope *envelope = lua_check_envelope(L, index);
    lua_rawgeti(L, LUA_REGISTRYINDEX, envelope->message_ref);
}

/*
 * Put a message (Table) inside an Envelope with the author and tone so it can
 * be sent to the right recipients.
 *
 * Envelope.new(author, tone, { "movement", 20, 40 })
 */
static int
lua_envelope_new (lua_State *L)
{
    int message_ref;
    Envelope *envelope;

    Actor *author = lua_check_actor(L, 1);
    const char *tone = luaL_checkstring(L, 2);
    envelope_check_table(L, 3);

    message_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    envelope = lua_newuserdata(L, sizeof(Envelope));
    luaL_getmetatable(L, ENVELOPE_LIB);
    lua_setmetatable(L, -2);

    envelope->author = author;
    envelope->tone = tone;
    envelope->message_ref = message_ref;
    envelope->mailbox = NULL;
    envelope->recipient = NULL;

    return 1;
}

static int
lua_envelope_tostring (lua_State *L)
{
    Envelope *envelope = lua_check_envelope(L, 1);
    lua_pushfstring(L, "%s %p", ENVELOPE_LIB, envelope);
    return 1;
}

/*
 * Return the message inside the envelope.
 */
static int
lua_envelope_message (lua_State *L)
{
    envelope_push_message(L, 1);
    return 1;
}

static const luaL_Reg envelope_methods[] = {
    {"message", lua_envelope_message},
    {"__tostring", lua_envelope_tostring},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Envelope (lua_State *L)
{
    return utils_lua_meta_open(L, ENVELOPE_LIB, envelope_methods, lua_envelope_new);
}
