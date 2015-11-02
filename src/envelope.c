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
 * An Envelope exists inside a Mailbox. It holds all the metadata of a message,
 * along with the message itself. Since our Actors have no use for the meta-
 * data, we can get by with having the Envelope exist in the Mailbox's 
 * lua_State and just copying the message table from the Mailbox to the Actor.
 *
 * Envelope.new(mailbox, author, tone, { "movement", 20, 40 })
 */
static int
lua_envelope_new (lua_State *L)
{
    int envelope_ref, length;
    lua_State *B;
    Envelope *envelope;
    Mailbox *box = lua_check_mailbox(L, 1);
    Actor *author = lua_check_actor(L, 2);
    const char *tone = luaL_checkstring(L, 3);
    envelope_check_table(L, 4);

    //B = box->L;
    B = mailbox_request_stack(box);
    envelope = lua_newuserdata(B, sizeof(Envelope));
    luaL_getmetatable(B, ENVELOPE_LIB);
    lua_setmetatable(B, -2);

    envelope->author = author;
    envelope->recipient = NULL;
    envelope->mailbox = box;
    envelope->tone = tone;
    box->envelope_count += 1;

    utils_copy_top(B, L);
    envelope->message_ref = luaL_ref(B, LUA_REGISTRYINDEX);

    envelope_ref = luaL_ref(B, LUA_REGISTRYINDEX);
    lua_rawgeti(B, LUA_REGISTRYINDEX, box->envelopes_ref);
    length = luaL_len(B, -1);
    lua_rawgeti(B, LUA_REGISTRYINDEX, envelope_ref);
    lua_rawseti(B, -2, length + 1);
    lua_pop(B, 1);

    luaL_unref(B, LUA_REGISTRYINDEX, envelope_ref);

    mailbox_return_stack(box);
    utils_push_object(L, envelope, ENVELOPE_LIB);
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
    Envelope *envelope = lua_check_envelope(L, 1);
    //lua_State *B = envelope->mailbox->L;
    lua_State *B = mailbox_request_stack(envelope->mailbox);
    lua_rawgeti(B, LUA_REGISTRYINDEX, envelope->message_ref);
    utils_copy_top(L, B);
    lua_pop(B, 1);
    mailbox_return_stack(envelope->mailbox);
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
