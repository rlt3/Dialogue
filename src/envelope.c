#include "envelope.h"
#include "utils.h"

struct Envelope {
    int table_reference;
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
 */
static int
lua_envelope_new (lua_State *L)
{
    int i, argc = lua_gettop(L);
    Envelope *envelope = lua_newuserdata(L, sizeof(Envelope));

    luaL_checkany(L, 1);
    lua_newtable(L);
    
    /* We push the raw value (no conversions) onto the array */
    for (i = 1; i <= argc; i++) {
        lua_pushvalue(L, i);
        lua_rawseti(L, -2, i);
    }

    /* this pops the new table we made */
    envelope->table_reference = luaL_ref(L, LUA_REGISTRYINDEX);

    /* push our Envelope userdata on top of the stack as an object */
    luaL_getmetatable(L, ENVELOPE_LIB);
    lua_setmetatable(L, -2);
    return 1;
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
luaopen_Envelope (lua_State *L)
{
    return lua_meta_open(L, ENVELOPE_LIB, envelope_methods, lua_envelope_new);
}
