#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dialogue.h"
#include "postman.h"
#include "mailbox.h"
#include "envelope.h"
#include "actor.h"
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

lua_State *
mailbox_request_stack (Mailbox *box)
{
    pthread_mutex_lock(&box->mutex);
    return box->L;
}

void
mailbox_return_stack (Mailbox *box)
{
    pthread_mutex_unlock(&box->mutex);
}

/*
 * Push the next envelope onto the top of the Mailbox stack.
 */
void
mailbox_push_next_envelope (Mailbox *box)
{
    lua_State *B = box->L;
    lua_getglobal(B, "table");
    lua_getfield(B, -1, "remove");
    lua_rawgeti(B, LUA_REGISTRYINDEX, box->envelopes_ref);
    lua_pushinteger(B, 1);
    lua_call(B, 2, 1);
    lua_pop(B, 2); /* pop 'table' and return value */
    box->envelope_count -= 1;
}

void
mailbox_assign_postman (Mailbox *box)
{
    lua_State *B = mailbox_request_stack(box);
    mailbox_push_next_envelope(box);
    /*
     * 1. Get the latest envelope
     * 2. Get the audience (list of actors) for the message
     * 3. Put the message on top of the stack.
     * 4. Map the audience to postman_give_address
     */
    mailbox_return_stack(box);
}

void *
mailbox_thread (void *arg)
{
    Mailbox *box = arg;

    while (box->processing) {
        while (box->paused)
            usleep(1000);

        while (box->envelope_count > 0)
            mailbox_assign_postman(box);
    }

    return NULL;
}

void
mailbox_free_postmen (Mailbox *box)
{
    int i;
    for (i = 0; i < box->postmen_count; i++) {
        if (box->postmen[i] == NULL)
            continue;

        postman_free(box->postmen[i]);
        box->postmen[i] = NULL;
    }

    free(box->postmen);
}

/*
 * Create the intermediary state which messages are held before sent to Actors.
 */
static int
lua_mailbox_new (lua_State *L)
{
    pthread_t thread;
    lua_State *B;
    int i, thread_count = luaL_checkinteger(L, 1);
    Mailbox *box = lua_newuserdata(L, sizeof(Mailbox));
    luaL_getmetatable(L, MAILBOX_LIB);
    lua_setmetatable(L, -2);

    box->processing = 1;
    box->paused = 0;
    box->envelope_count = 0;

    box->postmen_count = thread_count;
    box->postmen = malloc(sizeof(Postman) * thread_count);
    if (box->postmen == NULL)
        luaL_error(L, "Error allocating memory for Mailbox threads!");

    for (i = 0; i < box->postmen_count; i++) {
        box->postmen[i] = postman_create(box);
        if (box->postmen[i] == NULL) {
            mailbox_free_postmen(box);
            luaL_error(L, "Error allocating memory for Postman!");
        }
    }

    box->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    box->L = luaL_newstate();
    B = box->L;

    luaL_openlibs(B);
    luaL_requiref(B, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(B, 1);

    lua_newtable(B);
    box->envelopes_ref = luaL_ref(B, LUA_REGISTRYINDEX);

    pthread_create(&thread, NULL, mailbox_thread, box);
    pthread_detach(thread);

    return 1;
}

/*
 * mailbox:add(author, tone, {"movement", 20, 40})
 */
static int
lua_mailbox_add (lua_State *L)
{
    lua_check_mailbox(L, 1);
    lua_check_actor(L, 2);
    luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Mailbox");
    lua_getfield(L, -1, "Envelope");
    lua_getfield(L, -1, "new");
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_pushvalue(L, 4);
    lua_call(L, 4, 1);

    return 1;
}

/*
 * Get the list of envelopes currently in the mailbox.
 */
static int
lua_mailbox_envelopes (lua_State *L)
{
    int i, table_index;
    lua_State *B;
    Mailbox *box = lua_check_mailbox(L, 1);
    Envelope *envelope;
    B = mailbox_request_stack(box);

    lua_newtable(L);
    
    lua_rawgeti(B, LUA_REGISTRYINDEX, box->envelopes_ref);
    table_index = lua_gettop(B);

    lua_pushnil(B);
    for (i = 1; lua_next(B, table_index); i++, lua_pop(B, 1)) {
        envelope = lua_check_envelope(B, -1);
        utils_push_object(L, envelope, ENVELOPE_LIB);
        lua_rawseti(L, -2, i);
    }
    lua_pop(B, 1);

    mailbox_return_stack(box);
    return 1;
}

static int
lua_mailbox_start (lua_State *L)
{
    Mailbox *box = lua_check_mailbox(L, 1);
    box->processing = 1;
    box->paused = 0;
    return 1;
}

static int
lua_mailbox_pause (lua_State *L)
{
    Mailbox *box = lua_check_mailbox(L, 1);
    box->paused = 1;
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
 * Stop thread and free any postmen before Lua garbage collects the mailbox.
 */
static int
lua_mailbox_gc (lua_State *L)
{
    Mailbox *box = lua_check_mailbox(L, 1);
    box->processing = 0;
    box->paused = 0;
    box->envelope_count = 0;
    usleep(1500);
    mailbox_free_postmen(box);
    lua_close(box->L);
    return 0;
}

static const luaL_Reg mailbox_methods[] = {
    {"add",        lua_mailbox_add},
    {"envelopes",  lua_mailbox_envelopes},
    {"start",      lua_mailbox_start},
    {"pause",      lua_mailbox_pause},
    {"__tostring", lua_mailbox_print},
    {"__gc",       lua_mailbox_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Mailbox (lua_State * L)
{
    return utils_lua_meta_open(L, MAILBOX_LIB, mailbox_methods, lua_mailbox_new);
}
