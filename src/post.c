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

    printf("Postman %p [BOOT]\n", postman);
    pthread_mutex_lock(&postman->lock);

    while (postman->working) {
        printf("Postman %p [SENDING] ...\n", postman);
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
        postman->waiting = 1;
        printf("Postman %p [WAITING] ...\n", postman);
        while (postman->waiting)
            pthread_cond_wait(&postman->wait_cond, &postman->lock);
    }

    printf("Postman %p [QUIT]\n", postman);
    pthread_mutex_unlock(&postman->lock);
    return NULL;
}

Postman *
postman_start ()
{
    lua_State *P;
    pthread_mutexattr_t mutex_attr;
    Postman *postman = malloc(sizeof(*postman));

    postman->working = 1;
    postman->waiting = 1;
    postman->author = NULL;
    postman->tone = NULL;
    postman->L = luaL_newstate();
    P = postman->L;
    luaL_openlibs(P);

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&postman->lock, &mutex_attr);
    postman->wait_cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    pthread_create(&postman->thread, NULL, postman_thread, postman);
    pthread_detach(postman->thread);

    return postman;
}

/*
 * Delivers the message on top of the Actor's stack.  The Post finds a free
 * Postman, copies the message from the Actor's stack to the Postman's
 * stack. Then it delivers it to the correct audience, given by the tone.
 */
void
post_deliver_actor_top (Post *post, Actor *author, const char *tone)
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
        pthread_cond_signal(&postman->wait_cond);

        break;
    }
}

Post *
lua_check_post (lua_State *L, int index)
{
    return (Post *) luaL_checkudata(L, index, POST_LIB);
}

/*
 * Create a new Post -- a collection of Postmen.
 */
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

/*
 * Wait and lock each of the Postmen. Then turn off their loops and signal it 
 * to come back on.
 */
int
lua_post_gc (lua_State *L)
{
    Post *post = lua_check_post(L, 1);
    int i, rc, lock_count = 0;

    for (i = 0; lock_count < post->postmen_count; i = i % post->postmen_count) {
        rc = pthread_mutex_trylock(&post->postmen[i]->lock);
        if (rc == EBUSY)
            continue;
        lock_count++;
    }

    for (i = 0; i < post->postmen_count; i++) {
        post->postmen[i]->waiting = 0;
        post->postmen[i]->working = 0;
        pthread_mutex_unlock(&post->postmen[i]->lock);
        pthread_cond_signal(&post->postmen[i]->wait_cond);
        usleep(1000);
        lua_close(post->postmen[i]->L);
        free(post->postmen[i]);
    }

    free(post->postmen);

    return 0;
}

int
lua_post_tostring (lua_State *L)
{
    Post *post = lua_check_post(L, 1);
    lua_pushfstring(L, "%s %p", POST_LIB, post);
    return 1;
}

static const luaL_Reg post_methods[] = {
    /*
    {"play",       lua_post_play},
    {"pause",      lua_post_pause},
    {"stop",       lua_post_stop},
    */
    {"__gc",       lua_post_gc},
    {"__tostring", lua_post_tostring},
    { NULL, NULL }
};

int 
luaopen_Dialogue_Post (lua_State *L)
{
    return utils_lua_meta_open(L, POST_LIB, post_methods, lua_post_new);
}
