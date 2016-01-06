#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "post.h"
#include "dialogue.h"
#include "postman.h"
#include "utils.h"
#include "luaf.h"

typedef struct Post {
    Postman **postmen;
    int postmen_count;
    int actors_per_postman;
    int seed;
} Post;


Post *
lua_check_post (lua_State *L, int index)
{
    return (Post *) luaL_checkudata(L, index, POST_LIB);
}

/*
 * Create the Post for the Dialogue. This function acts as a Singleton for the
 * Post as we want only one running at a time. The number of threads should 
 * match the number of cores of the CPU. So, if you have 2 cores, you need 2
 * threads. Having more threads than necessary can actually *hurt* performance.
 *
 * Dialogue.Post.new(thread count, actors per thread)
 */
int
lua_post_init (lua_State *L)
{
    Post *post;
    int postmen_count = luaL_optinteger(L, 1, 4);
    int actors_per_thread = luaL_optinteger(L, 2, 1024);
    int post_index = lua_gettop(L) + 1;
    int i;

    post = lua_newuserdata(L, sizeof(*post));
    luaL_getmetatable(L, POST_LIB);
    lua_setmetatable(L, -2);

    post->postmen_count = postmen_count;
    post->actors_per_postman = actors_per_thread;
    post->postmen = malloc(sizeof(Postman*) * postmen_count);
    post->seed = time(NULL);
    srand(post->seed);

    printf("CREATE Post: %p\n", post);

    if (post->postmen == NULL)
        luaL_error(L, "Not enough memory to create Postmen");

    for (i = 0; i < postmen_count; i++) {
        post->postmen[i] = postman_create(L, post);
        printf("    CREATE Postman: %p\n", post->postmen[i]);
    }

    /* Append to the Dialogue tree the object for all `methods' to use */
    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Post");
    lua_pushvalue(L, post_index);
    lua_setfield(L, -2, "__obj");
    lua_pop(L, 2);
    
    return 1;
}

int
lua_post_gc (lua_State *L)
{
    Post *post = lua_check_post(L, 1);
    int i;

    printf("DELETE Post: %p\n", post);

    for (i = 0; i < post->postmen_count; i++) {
        printf("    DELETE Postman: %p\n", post->postmen[i]);
        postman_stop(post->postmen[i]);
    }

    free(post->postmen);

    return 0;
}

Post *
lua_getpost (lua_State *L)
{
    Post *post;
    lua_getglobal(L, "Dialogue");
    lua_getfield(L, -1, "Post");
    lua_getfield(L, -1, "__obj");
    post = lua_touserdata(L, -1);
    lua_pop(L, 3);
    return post;
}

/*
 * The Post handles mail in aggregate. It doesn't make sense that an 
 * asynchronous execution should have to get bottlenecked. It is a thread safe
 * method for sending Envelopes.
 *
 * Envelopes are just tables of data in a specific form. In this case, the form
 * is:
 *      { Actor , Action [, arg1 [, arg2 ...]] }
 *  Where arg1..argn may be yet another Envelope-like sequence. The Action of
 *  sending a message is implemented like this:
 *      { player, "send", "damage", "10" }
 *  Just like the table for the Post itself, uses this form, the Envelopes the
 *  Actors use have the same form, just without the needless preprended author
 *  -- but the Post already has that information.
 *
 *  Dialogue.Post.send(actor, action, ...)
 */
int
lua_post_send (lua_State *L)
{
    Post *post = lua_getpost(L);
    int args = lua_gettop(L);
    int start = rand() % post->postmen_count;
    int i;

    lua_newtable(L);
    for (i = 1; i <= args; i++) {
        lua_pushvalue(L, i);
        lua_rawseti(L, -2, i);
    }

    for (i = start; i < post->postmen_count; i = (i + 1) % post->postmen_count)
        if (mailbox_send_lua_top(post->postmen[i]->mailbox, L))
            break;

    return 1;
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
    /* create metatable */
    luaL_newmetatable(L, POST_LIB);

    /* metatable.__index = metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    /* register methods */
    luaL_setfuncs(L, post_methods, 0);

    lua_newtable(L);
    lua_pushcfunction(L, lua_post_init);
    lua_setfield(L, -2, "init");

    lua_pushcfunction(L, lua_post_send);
    lua_setfield(L, -2, "send");

    return 1;
}
