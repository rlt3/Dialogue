#include "envelope.h"
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
Envelope *
envelope_create_empty()
{
    static Envelope envelope;
    envelope.next = NULL;
    envelope.data = NULL;
    envelope.data_len = 0;
    return &envelope;
}

/*
 * Determine if an envelope should be called like f(envelope) or not.
 */
void
envelope_bind(Envelope *envelope, void (*f) (Envelope *))
{
    if (envelope->data_len > 0)
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
        lua_pushstring(L, envelope->data[0]);
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
    int len, i, bytes;
    Envelope *envelope;

    luaL_checktype(L, 1, LUA_TTABLE);
    len = luaL_len(L, 1);
    luaL_argcheck(L, len > 0, 1, "Message needs to have a title!");

    bytes = sizeof(Envelope) + (len - 1) * sizeof(const char*);
    envelope = lua_newuserdata(L, bytes);
    luaL_getmetatable(L, ENVELOPE_LIB);
    lua_setmetatable(L, -2);

    envelope->next = NULL;
    envelope->data_len = len - 1;

    lua_pushnil(L);
    for (i = 0; lua_next(L, 1); i++, lua_pop(L, 1)) {
        envelope->data[i] = lua_tostring(L, -1);
    }

    return 1;
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

static const luaL_Reg envelope_methods[] = {
    {"table", lua_envelope_table},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Envelope (lua_State *L)
{
    return lua_meta_open(L, ENVELOPE_LIB, envelope_methods, lua_envelope_new);
}
