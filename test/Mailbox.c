#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Postman.h"
#include "utils.h"

#define MAILBOX_LIB "Mailbox"

typedef struct Mailbox {
    lua_State *L;
    int processing;
    pthread_mutex_t mutex;
    pthread_cond_t new_envelope;
    pthread_t thread;
    struct Postman **postmen;
    int postmen_count;
    int envelopes_table;
    int envelope_count;
} Mailbox;

/*
 * Make sure the argument at index N is a Mailbox and return it if it is.
 */
Mailbox *
lua_check_mailbox (lua_State *L, int index)
{
    return (Mailbox*) luaL_checkudata(L, index, MAILBOX_LIB);
}

void
mailbox_handle_next (Mailbox *mailbox)
{
    int table_index;

    lua_State *B = mailbox->L;
    lua_getglobal(B, "table");
    lua_getfield(B, -1, "remove");
    lua_rawgeti(B, LUA_REGISTRYINDEX, mailbox->envelopes_table);
    lua_pushinteger(B, 1);
    lua_call(B, 2, 1);
    
    mailbox->envelope_count--;
    table_index = lua_gettop(B);

    printf("{ ");
    lua_pushnil(B);
    while (lua_next(B, table_index)) {
        printf("%s ", lua_tostring(B, -1));
        lua_pop(B, 1);
    }
    printf("}\n");

    lua_pop(B, 2);
}

void *
mailbox_thread (void *arg)
{
    int rc;
    Mailbox *mailbox = arg;

    rc = pthread_mutex_lock(&mailbox->mutex);

    while (mailbox->processing) {
        if (mailbox->envelope_count > 0) {
            printf("Processing envelope\n");
            mailbox_handle_next(mailbox);
        } else {
            printf("Waiting on envelopes\n");
            rc = pthread_cond_wait(&mailbox->new_envelope, &mailbox->mutex);
        }
    }

    return NULL;
}

static int
lua_mailbox_new (lua_State *L)
{
    //int i;
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

    //for (i = 0; i < box->postmen_count; i++)
    //    postman_create(L, box->postmen[i], box);
    
    mailbox->L = luaL_newstate();
    B = mailbox->L;

    luaL_openlibs(B);
    lua_newtable(B);
    mailbox->envelopes_table = luaL_ref(B, LUA_REGISTRYINDEX);
    mailbox->envelope_count = 0;

    mailbox->processing = 1;
    mailbox->new_envelope = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
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

    rc = pthread_mutex_lock(&mailbox->mutex);
    B = mailbox->L;

    mailbox->envelope_count++;

    lua_rawgeti(B, LUA_REGISTRYINDEX, mailbox->envelopes_table);
    utils_copy_top(B, L);
    lua_rawseti(B, -2, mailbox->envelope_count);
    lua_pop(B, 1);

    lua_pushinteger(L, mailbox->envelope_count);

    rc = pthread_mutex_unlock(&mailbox->mutex);
    rc = pthread_cond_signal(&mailbox->new_envelope);

    return 1;
}

static int
lua_mailbox_gc (lua_State *L)
{
    int rc;
    Mailbox *mailbox = lua_check_mailbox(L, 1);

    rc = pthread_mutex_lock(&mailbox->mutex);
    mailbox->processing = 0;
    rc = pthread_mutex_unlock(&mailbox->mutex);

    free(mailbox->postmen);
    lua_close(mailbox->L);

    return 0;
}

static const luaL_Reg mailbox_methods[] = {
    {"add",        lua_mailbox_add},
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
