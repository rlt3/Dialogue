#include <stdio.h>
#include <unistd.h>
#include "mailbox.h"
#include "post.h"
#include "utils.h"

/*
 * Make sure the argument at index N is a Mailbox and return it if it is.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index)
{
    return (Mailbox*) luaL_checkudata(L, index, MAILBOX_LIB);
}

/*
 * Create the intermediary state which messages are held before sent to Actors.
 */
static int
lua_mailbox_new (lua_State *L)
{
    lua_State *B;
    Mailbox *box = lua_newuserdata(L, sizeof(Mailbox));
    luaL_getmetatable(L, MAILBOX_LIB);
    lua_setmetatable(L, -2);

    box->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    box->L = luaL_newstate();
    B = box->L;

    luaL_openlibs(B);
    lua_newtable(B);
    box->envelopes_ref = luaL_ref(B, LUA_REGISTRYINDEX);

    return 1;
}

static int
lua_mailbox_add (lua_State *L)
{
    int length;
    lua_State *B;
    Mailbox *box = lua_check_mailbox(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    B = box->L;
    lua_rawgeti(B, LUA_REGISTRYINDEX, box->envelopes_ref);
    length = luaL_len(B, -1);
    utils_copy_top(B, L);
    lua_rawseti(B, -2, length + 1);
    lua_pop(B, 1);

    return 0;
}

static int
lua_mailbox_envelopes (lua_State *L)
{
    lua_State *B;
    Mailbox *box = lua_check_mailbox(L, 1);
    B = box->L;
    
    lua_rawgeti(B, LUA_REGISTRYINDEX, box->envelopes_ref);
    utils_copy_top(L, B);
    lua_pop(B, 1);

    return 1;
}

static int
lua_mailbox_print (lua_State *L)
{
    Mailbox *box = lua_check_mailbox(L, 1);
    lua_pushfstring(L, "%s %p", MAILBOX_LIB, box);
    return 1;
}

/*
 * Stop thread and free any envelopes before Lua garbage collects the mailbox.
 */
static int
lua_mailbox_gc (lua_State *L)
{
    Mailbox *box = lua_check_mailbox(L, 1);
    lua_close(box->L);
    return 0;
}

static const luaL_Reg mailbox_methods[] = {
    {"add",        lua_mailbox_add},
    {"envelopes",  lua_mailbox_envelopes},
    {"__tostring", lua_mailbox_print},
    {"__gc",       lua_mailbox_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Mailbox (lua_State * L)
{
    return utils_lua_meta_open(L, MAILBOX_LIB, mailbox_methods, lua_mailbox_new);
}
