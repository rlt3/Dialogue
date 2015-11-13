#include "Mailbox.h"
#include "Postman.h"
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

static int
lua_mailbox_new (lua_State *L)
{
    int i;
    int thread_count;
    pthread_mutexattr_t mutex_attr;
    lua_State *B;
    Mailbox *mailbox;

    thread_count = luaL_checkinteger(L, 1);

    mailbox = lua_newuserdata(L, sizeof(Mailbox));
    luaL_getmetatable(L, MAILBOX_LIB);
    lua_setmetatable(L, -2);

    mailbox->postmen_count = thread_count;
    mailbox->postmen = malloc(sizeof(Postman*) * thread_count);

    if (mailbox->postmen == NULL)
        luaL_error(L, "Error allocating memory for Mailbox threads!");

    for (i = 0; i < mailbox->postmen_count; i++) {
        mailbox->postmen[i] = postman_new(mailbox);
        if (mailbox->postmen[i] == NULL) {
            luaL_error(L, "Error allocating memory for Postman!");
        }
    }
    
    mailbox->L = luaL_newstate();
    B = mailbox->L;

    luaL_openlibs(B);
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
 * Copy the envelope (a table) to the internal state. Returns the current
 * number of envelopes the mailbox has.
 */
static int
lua_mailbox_add (lua_State *L)
{
    int rc;
    lua_State *B;
    Mailbox *mailbox = lua_check_mailbox(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    /* Wait & lock for access to the internal state */
    rc = pthread_mutex_lock(&mailbox->mutex);
    B = mailbox->L;

    mailbox->envelope_count++;

    /* Copy table from the global lua State (L) to the internal (B) */
    lua_rawgeti(B, LUA_REGISTRYINDEX, mailbox->envelopes_table);
    utils_copy_top(B, L);
    lua_rawseti(B, -2, mailbox->envelope_count);
    lua_pop(B, 1);

    lua_pushinteger(L, mailbox->envelope_count);

    /* Unlock access and then signal thread the wait condition */
    rc = pthread_mutex_unlock(&mailbox->mutex);
    rc = pthread_cond_signal(&mailbox->new_envelope);

    return 1;
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
    {"__gc",       lua_mailbox_gc},
    { NULL, NULL }
};

int
luaopen_Mailbox (lua_State *L)
{
    luaL_newmetatable(L, MAILBOX_LIB);

    /* metatable.__index = metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    /* register methods */
    luaL_setfuncs(L, mailbox_methods, 0);

    lua_newtable(L);
    lua_pushcfunction(L, lua_mailbox_new);
    lua_setfield(L, -2, "new");

    return 1;
}
