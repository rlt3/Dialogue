#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "post.h"
#include "tone.h"
#include "utils.h"

/*
 * Assume there is a message, which is a table, at index 1 each loop. We send 
 * that message to each Actor in an audience determined by the tone. When we
 * get done sending the message to the mailbox, we 
 */
void *
postman_thread (void *arg)
{
    Actor *recipient;
    Postman *postman = arg;
    lua_State *P = postman->L;
    const int message_index = 1;
    const int audience_index = 2;

    pthread_mutex_trylock(&postman->lock);

    while (postman->working) {
        if (!lua_istable(P, message_index) || 
            postman->author == NULL || 
            postman->tone == NULL)
            goto wait_for_work;

        audience_filter_tone(P, postman->author, postman->tone);

        lua_pushnil(P);
        while (lua_next(P, audience_index)) {
            recipient = lua_check_actor(P, -1);
            lua_pushvalue(P, message_index);
            mailbox_send(recipient->mailbox, postman->author, P);
            lua_pop(P, 2); /* key & message table */
        }
        lua_pop(P, 2); /* audience table & message */

wait_for_work:
        postman->author = NULL;
        postman->tone = NULL;
        while (postman->author == NULL)
            pthread_cond_wait(&postman->work_cond, &postman->lock);
    }

    return NULL;
}

Postman *
postman_start ()
{
    lua_State *P;
    pthread_mutexattr_t mutex_attr;
    Postman *postman = malloc(sizeof(*postman));

    postman->working = 1;
    postman->author = NULL;
    postman->tone = NULL;
    postman->L = luaL_newstate();
    P = postman->L;
    luaL_openlibs(P);

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&postman->lock, &mutex_attr);
    postman->work_cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    pthread_create(&postman->thread, NULL, postman_thread, postman);
    pthread_detach(postman->thread);

    return postman;
}

/*
 * Expects a message on top of the Actor's Lua stack. The Post finds a free
 * Postman and creates an Envelope in the Postman's state and the Postman
 * delivers it.
 */
void
post_deliver_lua_top (Post *post, Actor *author, const char *tone)
{
    Postman *postman;
    lua_State *A = author->L;
    int i, rc;

    if (post == NULL || post->postmen == NULL)
        return;

    for (i = 0; i < post->postmen_count; i = i % post->postmen_count) {
        rc = pthread_mutex_trylock(&post->postmen[i]->lock);

        if (rc == EBUSY)
            continue;

        postman = post->postmen[i];

        utils_copy_top(postman->L, A);
        postman->author = author;
        postman->tone = tone;

        pthread_mutex_unlock(&postman->lock);
        pthread_cond_signal(&postman->work_cond);

        break;
    }
}

int
lua_post_new (lua_State *L)
{
    Post *post;
    int i, postmen_count = luaL_checkinteger(L, 1);

    post = lua_newuserdata(L, sizeof(*post));
    luaL_getmetatable(L, POST_LIB);
    lua_setmetatable(L, -2);

    post->postmen_count = 0;
    post->postmen = malloc(sizeof(Postman*) * postmen_count);

    if (post->postmen == NULL)
        luaL_error(L, "Not enough memory to create Postmen");

    for (i = 0; i < postmen_count; i++)
        post->postmen[i] = postman_start();
    
    return 1;
}

static const luaL_Reg post_methods[] = {
    /*
    {"play",       lua_actor_play},
    {"pause",      lua_actor_pause},
    {"stop",       lua_actor_stop},
    {"__gc",       lua_actor_gc},
    {"__tostring", lua_actor_tostring},
    */
    { NULL, NULL }
};

int 
luaopen_Dialogue_Post (lua_State *L)
{
    return utils_lua_meta_open(L, POST_LIB, post_methods, lua_post_new);
}
