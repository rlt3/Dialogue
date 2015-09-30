#include "envelope.h"
#include "utils.h"

struct Envelope {
    int table_reference;
    int length;
};

/*
 * Make sure userdata at index N is an Envelope.
 */
Envelope *
lua_check_envelope (lua_State *L, int index)
{
    return (Envelope *) luaL_checkudata(L, index, ENVELOPE_LIB);
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

    /* pop and reference the table given to us */
    reference = luaL_ref(L, LUA_REGISTRYINDEX);

    /* create and push our envelope */
    envelope = lua_newuserdata(L, sizeof(Envelope));
    envelope->table_reference = reference;
    envelope->length = len;
    luaL_getmetatable(L, ENVELOPE_LIB);
    lua_setmetatable(L, -2);
    return 1;
}

/*
 * Checks for an Envelope at index.
 * Push an envelope's table onto the stack.
 */
void
envelope_push_table (lua_State *L, int index)
{
    Envelope *envelope = lua_check_envelope(L, index);
    lua_rawgeti(L, LUA_REGISTRYINDEX, envelope->table_reference);
}

/*
 * Push the title of an envelope at index onto the top of the stack.
 */
void
envelope_push_title (lua_State *L, int index)
{
    lua_rawgeti(L, index, 1);
}

/*
 * Push the envelope (at index) data sequentially onto the stack.
 * Returns the number of args pushed.
 */
int
envelope_push_data (lua_State *L, int index)
{
    int i, len = luaL_len(L, index);

    /* first element in an envelope table is the title */
    for (i = 2; i <= len; i++)
        lua_rawgeti(L, index, i);

    return len - 1;
}

/*
 * Return the raw table that the envelope operates on.
 */
static int
lua_envelope_table (lua_State *L)
{
    Envelope *envelope = lua_check_envelope(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, envelope->table_reference);
    return 1;
}

/*
 * Get the title of the message inside the envelope.
 */
static int
lua_envelope_title (lua_State *L)
{
    Envelope *envelope = lua_check_envelope(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, envelope->table_reference);
    lua_rawgeti(L, -1, 1);
    return 1;
}

/*
 * Get the data for the message.
 */
static int
lua_envelope_data (lua_State *L)
{
    int i, len;
    Envelope *envelope = lua_check_envelope(L, 1);

    lua_rawgeti(L, LUA_REGISTRYINDEX, envelope->table_reference);
    len = luaL_len(L, 2);

    /* t = {} */
    lua_newtable(L);

    for (i = 2; i <= len; i++) {
        /* t[i-1] = envelope[i] */
        lua_rawgeti(L, 2, i);
        lua_rawseti(L, 3, i - 1);
    }

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
    {"title", lua_envelope_title},
    {"data",  lua_envelope_data},
    {"table", lua_envelope_table},
    {"__gc",  lua_envelope_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Envelope (lua_State *L)
{
    return lua_meta_open(L, ENVELOPE_LIB, envelope_methods, lua_envelope_new);
}
