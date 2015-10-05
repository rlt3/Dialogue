#include "envelope.h"
#include "utils.h"

/*
 * Make sure userdata at index N is an Envelope.
 */
Envelope *
lua_check_envelope (lua_State *L, int index)
{
    return (Envelope *) luaL_checkudata(L, index, ENVELOPE_LIB);
}

/*
 * Push the table of an Envelope at index.
 */
void
envelope_push_table (lua_State *L, int index)
{
    Envelope *envelope = lua_check_envelope(L, index);
    lua_rawgeti(L, LUA_REGISTRYINDEX, envelope->table_reference);
}

/*
 * Create a new envelope with the given message.
 * Envelope{ "update", 20 }
 * Envelope{ "location", 1, 2 }
 */
static int
lua_envelope_new (lua_State *L)
{
    int len, reference;
    Envelope *envelope;

    luaL_checktype(L, 1, LUA_TTABLE);
    len = luaL_len(L, 1);
    luaL_argcheck(L, len > 0, 1, "Message needs to have a title!");

    reference = luaL_ref(L, LUA_REGISTRYINDEX);

    envelope = lua_newuserdata(L, sizeof(Envelope));
    luaL_getmetatable(L, ENVELOPE_LIB);
    lua_setmetatable(L, -2);

    envelope->table_reference = reference;
    envelope->L = L;

    return 1;
}

/*
 * Return the raw table that the envelope operates on.
 */
static int
lua_envelope_table (lua_State *L)
{
    envelope_push_table(L, 1);
    return 1;
}

/*
 * Clear our references when getting garbage collected.
 */
static int
lua_envelope_gc (lua_State *L)
{
    Envelope *envelope = lua_check_envelope(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, envelope->table_reference);
    return 0;
}

static const luaL_Reg envelope_methods[] = {
    {"table", lua_envelope_table},
    {"__gc",  lua_envelope_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Envelope (lua_State *L)
{
    return lua_meta_open(L, ENVELOPE_LIB, envelope_methods, lua_envelope_new);
}
