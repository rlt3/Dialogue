#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dialogue.h"
#include "postman.h"
#include "mailbox.h"
#include "envelope.h"
#include "actor.h"
#include "post.h"
#include "tone.h"
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
 * Removes the next envelope from its queue of Envelopes and leaves it at the top
 * of the Mailbox stack.
 */
void
mailbox_push_next_envelope (Mailbox *mailbox)
{
    int ref;
    lua_State *B = mailbox->L;
    lua_getglobal(B, "table");
    lua_getfield(B, -1, "remove");
    lua_rawgeti(B, LUA_REGISTRYINDEX, mailbox->envelopes_table);
    lua_pushinteger(B, 1);
    lua_call(B, 2, 1);
    ref = luaL_ref(B, LUA_REGISTRYINDEX);
    lua_pop(B, 1);
    lua_rawgeti(B, LUA_REGISTRYINDEX, ref);
    luaL_unref(B, LUA_REGISTRYINDEX, ref);
}

/*
 * Loop through the postmen until one is found that can deliver.
 */
void
mailbox_alert_postman (Mailbox *mailbox)
{
    int i;

    for (i = 0; i < mailbox->postmen_count; i++) {
        if (postman_get_address(mailbox->postmen[i]))
            break;

        if (i == mailbox->postmen_count - 1)
            i = -1;
    }
}

void *
mailbox_thread (void *arg)
{
    int rc;
    Mailbox *mailbox = arg;

    rc = pthread_mutex_lock(&mailbox->mutex);

    while (mailbox->processing) {
        if (mailbox->envelope_count > 0) {
            mailbox_alert_postman(mailbox);
            mailbox->envelope_count--;
        } else {
            rc = pthread_cond_wait(&mailbox->new_envelope, &mailbox->mutex);
        }
    }

    return NULL;
}

/*
 * Create the Mailbox which holds the messages being sent inside Envelopes. It
 * holds these Envelopes inside an intermediary Lua stack and uses Postmen to
 * deliver them to Actors.
 */
static int
lua_mailbox_new (lua_State *L)
{
    int i;
    int thread_count;
    pthread_mutexattr_t mutex_attr;
    lua_State *B;
    Mailbox *mailbox;

    thread_count = luaL_checkinteger(L, 1);

    mailbox = lua_newuserdata(L, sizeof *mailbox);
    luaL_getmetatable(L, MAILBOX_LIB);
    lua_setmetatable(L, -2);

    mailbox->postmen_count = thread_count;
    mailbox->postmen = malloc(sizeof(*mailbox->postmen) * thread_count);

    if (mailbox->postmen == NULL)
        luaL_error(L, "Error allocating memory for Mailbox threads!");

    /* 
     * Allocate all the Postmen. If there's an error allocating one, free any
     * previously allocated Postmen and then their array and error out.
     */
    for (i = 0; i < mailbox->postmen_count; i++) {
        mailbox->postmen[i] = postman_new(mailbox);

        if (mailbox->postmen[i] == NULL) {
            for (i = i - 1; i >= 0; i--)
                postman_free(mailbox->postmen[i]);
            free(mailbox->postmen);
            luaL_error(L, "Error allocating memory for Postman!");
        }
    }
    
    mailbox->L = luaL_newstate();
    B = mailbox->L;
    luaL_openlibs(B);

    luaL_requiref(B, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(B, 1);

    lua_newtable(B);
    mailbox->envelopes_table = luaL_ref(B, LUA_REGISTRYINDEX);
    mailbox->envelope_count = 0;

    mailbox->processing = 1;
    mailbox->new_envelope = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    pthread_mutexattr_init(&mutex_attr);
    //pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mailbox->mutex, &mutex_attr);

    pthread_create(&mailbox->thread, NULL, mailbox_thread, mailbox);
    pthread_detach(mailbox->thread);

    return 1;
}

/*
 * Copy the message (the table) to the internal state and create an Envelope
 * for it. Put that envelope in the envelopes table and signal the thread to
 * process it if not already. Returns current number of Envelopes.
 *
 * mailbox:add(author, tone, {"movement", 20, 40})
 */
static int
lua_mailbox_add (lua_State *L)
{
    int rc, args, table_index;
    lua_State *B;
    Mailbox *mailbox;
    Actor *actor;
    const char *tone;

    /*
     * Because we want multiple Lua stacks (each actor, Interpreter) to be able
     * to use this Mailbox method, we must pass the Mailbox pointer between
     * stacks as light user data. Lua cannot differentiate between heavy user
     * data and light user data, so we can't check the type explicitly here.
     */

    args = lua_gettop(L);
    if (args != 4)
        luaL_error(L, "Incorrect # of arguments to Mailbox:add (%d, expected 4)", args);

    if (!lua_islightuserdata(L, 1))
        luaL_error(L, "Argument #1 to Mailbox:add needs to be Mailbox userdata");

    mailbox = (Mailbox*) lua_touserdata(L, 1);

    if (mailbox == NULL)
        luaL_error(L, "Mailbox reference cannot be NULL to Mailbox:add");

    actor = lua_check_actor(L, 2);
    tone = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    /* Wait & lock for access to the internal state */
    rc = pthread_mutex_lock(&mailbox->mutex);
    B = mailbox->L;

    mailbox->envelope_count++;

    lua_rawgeti(B, LUA_REGISTRYINDEX, mailbox->envelopes_table);
    table_index = lua_gettop(B);

    lua_getglobal(B, "Dialogue");
    lua_getfield(B, -1, "Mailbox");
    lua_getfield(B, -1, "Envelope");
    lua_getfield(B, -1, "new");
    utils_push_object(B, actor, ACTOR_LIB);
    lua_pushstring(B, tone);
    utils_copy_top(B, L);
    lua_call(B, 3, 1);
    
    lua_rawseti(B, table_index, luaL_len(B, table_index) + 1);

    lua_pop(B, 3); /* Pop Dialogue, Mailbox & Envelope */

    /* Unlock access and then signal thread the wait condition */
    rc = pthread_mutex_unlock(&mailbox->mutex);
    rc = pthread_cond_signal(&mailbox->new_envelope);

    return 0;
}

static int
lua_mailbox_count (lua_State *L)
{
    int rc;
    Mailbox *mailbox = lua_check_mailbox(L, 1);
    rc = pthread_mutex_lock(&mailbox->mutex);
    lua_pushinteger(L, mailbox->envelope_count);
    rc = pthread_mutex_unlock(&mailbox->mutex);
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
 * Stop the Mailbox thread and all the Postmen threads.
 */
static int
lua_mailbox_gc (lua_State *L)
{
    int rc, i;
    Mailbox *mailbox = lua_check_mailbox(L, 1);

    /* Wait for access (make sure nothing's processing) and stop the thread */
    rc = pthread_mutex_lock(&mailbox->mutex);
    mailbox->processing = 0;
    rc = pthread_mutex_unlock(&mailbox->mutex);

    for (i = 0; i < mailbox->postmen_count; i++)
        postman_free(mailbox->postmen[i]);

    free(mailbox->postmen);
    lua_close(mailbox->L);

    return 0;
}

static const luaL_Reg mailbox_methods[] = {
    {"add",        lua_mailbox_add},
    {"count",      lua_mailbox_count},
    {"__tostring", lua_mailbox_print},
    {"__gc",       lua_mailbox_gc},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Mailbox (lua_State * L)
{
    return utils_lua_meta_open(L, MAILBOX_LIB, mailbox_methods, lua_mailbox_new);
}
