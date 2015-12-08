#include "mailbox.h"
#include "script.h"
#include "utils.h"

/*
 * Check for a Mailbox at index. Errors if it isn't a Mailbox.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index)
{
    return (Mailbox *) luaL_checkudata(L, index, MAILBOX_LIB);
}

int
lua_mailbox_new (lua_State *L)
{
    lua_State *B;
    pthread_mutexattr_t mutex_attr;
    Mailbox *mailbox;
    Actor *actor = lua_check_actor(L, -1);

    mailbox = lua_newuserdata(L, sizeof(*mailbox));
    luaL_getmetatable(L, MAILBOX_LIB);
    lua_setmetatable(L, -2);

    mailbox->envelope_count = 0;
    mailbox->actor = actor;
    mailbox->L = luaL_newstate();

    B = mailbox->L;
    luaL_openlibs(B);

    lua_newtable(B);
    lua_setglobal(B, "__envelopes");

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mailbox->mutex, &mutex_attr);

    return 1;
}

/*
 * Block for the Mailbox. Expects a message on top of given stack. Copy message
 * from top of stack to the Mailbox's queue.
 */
void
mailbox_send (Mailbox *mailbox, lua_State *L)
{
    lua_State *B = mailbox->L;

    pthread_mutex_lock(&mailbox->mutex);
    lua_getglobal(B, "__envelopes");
    utils_copy_top(B, L);
    lua_rawseti(B, 1, mailbox->envelope_count + 1);
    lua_pop(B, 1);
    mailbox->envelope_count++;
    pthread_mutex_unlock(&mailbox->mutex);
}

/*
 * Assumes Mailbox mutex is acquired. Removes the next envelope from its queue
 * of Envelopes and leaves it at the top of the Mailbox stack. Can leave nil if
 * no envelopes.
 */
void
mailbox_push_next_envelope (Mailbox *mailbox)
{
    lua_State *B = mailbox->L;

    lua_getglobal(B, "table");
    lua_getfield(B, -1, "remove");
    lua_getglobal(B, "__envelopes");
    lua_pushinteger(B, 1);
    lua_call(B, 2, 1);

    lua_insert(B, lua_gettop(B) - 1);
    lua_pop(B, 1);

    mailbox->envelope_count--;
}

int
lua_mailbox_gc (lua_State *L)
{
    Mailbox *mailbox = lua_check_mailbox(L, 1);
    pthread_mutex_lock(&mailbox->mutex);
    lua_close(mailbox->L);
    pthread_mutex_unlock(&mailbox->mutex);
    return 0;
}

static const luaL_Reg mailbox_methods[] = {
    {"__gc", lua_mailbox_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Actor_Mailbox (lua_State *L)
{
    return utils_lua_meta_open(L, MAILBOX_LIB, mailbox_methods, lua_mailbox_new);
}
