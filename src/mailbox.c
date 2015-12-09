#include "mailbox.h"
#include "envelope.h"
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
    lua_getglobal(B, "Dialogue");
    lua_getfield(B, -1, "Actor");
    lua_getfield(B, -1, "Mailbox");
    lua_getfield(B, -1, "Envelope");

    lua_getglobal(B, "__envelopes");
    lua_getfield(B, -2, "new");
    utils_push_object(B, mailbox->actor, ACTOR_LIB);
    utils_copy_top(B, L);

    if (lua_pcall(B, 2, 1, 0)) {
        lua_pop(L, 4);
        goto cleanup;
    }

    lua_rawseti(B, -2, mailbox->envelope_count + 1);
    lua_pop(B, 5);
    mailbox->envelope_count++;

cleanup:
    pthread_mutex_unlock(&mailbox->mutex);
}

/*
 * Assumes Mailbox mutex is acquired. Removes the next Envelope from its queue
 * of Envelopes. It gets the Author information and pushes the message and
 * leaves it on top of the stack. Returns author pointer.
 */
Actor *
mailbox_push_next_envelope (Mailbox *mailbox)
{
    Actor *author;
    Envelope *envelope;
    int message_ref;
    lua_State *B = mailbox->L;

    /* remove like queue LIFO */
    lua_getglobal(B, "table");
    lua_getfield(B, -1, "remove");
    lua_getglobal(B, "__envelopes");
    lua_pushinteger(B, 1);
    lua_call(B, 2, 1);

    /* move the 'table' on top to pop */
    lua_insert(B, lua_gettop(B) - 1);
    lua_pop(B, 1);

    /* get all the relevant info before popping and gcing envelope */
    envelope = lua_check_envelope(B, -1);
    message_ref = envelope->message_ref;
    author = envelope->author;
    lua_pop(B, 1);

    /* finally push table and unref it */
    lua_rawgeti(B, LUA_REGISTRYINDEX, message_ref);
    luaL_unref(B, LUA_REGISTRYINDEX, message_ref);

    mailbox->envelope_count--;

    return envelope->author;
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
    utils_lua_meta_open(L, MAILBOX_LIB, mailbox_methods, lua_mailbox_new);

    luaL_requiref(L, ENVELOPE_LIB, luaopen_Dialogue_Actor_Mailbox_Envelope, 1);
    lua_setfield(L, -2, "Envelope");

    return 1;
}
