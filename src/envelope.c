#include <stdlib.h>
#include "envelope.h"
#include "post.h"
#include "actor.h"
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
 * Create an envelope that will fail a bind call.
 */
Envelope
envelope_create_empty()
{
    Envelope envelope;
    envelope.next = NULL;
    envelope.data = NULL;
    envelope.data_len = 0;
    return envelope;
}

/*
 * Determine if an envelope should be called like f(envelope) or not.
 */
void
envelope_bind(Envelope envelope, void (*f) (Envelope))
{
    if (envelope.data_len > 0)
        f(envelope);
}

/*
 * Push the table of an Envelope onto given lua_State.
 */
void
envelope_push_table (lua_State *L, Envelope *envelope)
{
    int i;
    lua_newtable(L);
    for (i = 0; i < envelope->data_len; i++) {
        lua_pushstring(L, envelope->data[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

/*
 * Create a new envelope with the given message.
 * Envelope{ "update", 20 }
 * Envelope{ "location", 1, 2 }
 */
static int
lua_envelope_new (lua_State *L)
{
    int len, i;
    Envelope *envelope;
    Actor *actor = NULL;
    Actor *recipient = NULL;
    Tone tone = post_tone_think;

    luaL_checktype(L, 1, LUA_TTABLE);
    len = luaL_len(L, 1);
    luaL_argcheck(L, len > 0, 1, "Message needs to have a title!");

    actor = lua_check_actor(L, 2);
    tone = lua_touserdata(L, 3);

    if (lua_gettop(L) == 4) {
        recipient = lua_check_actor(L, 3);
    }

    /* 
     * Instead of Lua handling this memory, we need to. This also commits us to
     * use all lightuserdata as Envelopes (bindings between actors).
     */
    envelope = malloc(sizeof(Envelope));
    lua_pushlightuserdata(L, envelope);
    luaL_getmetatable(L, ENVELOPE_LIB);
    lua_setmetatable(L, -2);

    envelope->tone = tone;
    envelope->author = actor;
    envelope->recipient = recipient;
    envelope->next = NULL;
    envelope->data_len = len;
    envelope->data = malloc(envelope->data_len * sizeof(const char*));

    lua_pushnil(L);
    for (i = 0; lua_next(L, 1); i++, lua_pop(L, 1))
        envelope->data[i] = lua_tostring(L, -1);

    return 1;
}

void
envelope_free (Envelope *envelope)
{
    free(envelope->data);
    free(envelope);
}

/*
 * Return the raw table that the envelope operates on.
 */
static int
lua_envelope_table (lua_State *L)
{
    Envelope *envelope = lua_check_envelope(L, 1);
    envelope_push_table(L, envelope);
    return 1;
}

static int
lua_envelope_gc (lua_State *L)
{
    Envelope *envelope = lua_check_envelope(L, 1);
    free(envelope->data);
    return 0;
}

static const luaL_Reg envelope_methods[] = {
    {"table", lua_envelope_table},
    {"__gc", lua_envelope_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Envelope (lua_State *L)
{
    return lua_meta_open(L, ENVELOPE_LIB, envelope_methods, lua_envelope_new);
}
