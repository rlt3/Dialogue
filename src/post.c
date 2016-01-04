#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "dialogue.h"
#include "postman.h"
#include "post.h"
#include "utils.h"
#include "luaf.h"

typedef struct Post {
    Postman **postmen;
    int postmen_count;
    int actors_per_postman;
} Post;


Post *
lua_check_post (lua_State *L, int index)
{
    return (Post *) luaL_checkudata(L, index, POST_LIB);
}

/*
 * Create the Post for the Dialogue.
 *
 * Dialogue.Post.new(postmen_count, actors per thread)
 *
 * This function turns the Dialogue.Post from a table into an object. It makes
 * it come alive, singleton style.
 */
int
lua_post_new (lua_State *L)
{
    Post *post;
    int postmen_count = luaL_optinteger(L, 1, 4);
    int actors_per_thread = luaL_optinteger(L, 2, 1024);
    int i;

    post = lua_newuserdata(L, sizeof(*post));
    luaL_getmetatable(L, POST_LIB);
    lua_setmetatable(L, -2);

    post->postmen_count = postmen_count;
    post->actors_per_postman = actors_per_thread;
    post->postmen = malloc(sizeof(Postman*) * postmen_count);

    if (post->postmen == NULL)
        luaL_error(L, "Not enough memory to create Postmen");

    for (i = 0; i < postmen_count; i++)
        post->postmen[i] = postman_create();

    /* Append to the Dialogue tree the object for all `methods' to use */
    luaf(L, "Dialogue.Post.__obj = %3");
    
    return 0;
}

Post *
lua_getpost (lua_State *L)
{
    Post *post;
    luaf(L, "return Dialogue.Post.__obj", 1);
    post = lua_touserdata(L, -1);
    lua_pop(L, 1);
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
    int i, args = lua_gettop(L);
    Post *post = lua_getpost(L);

    /*
     * TODO:
     *      Can parse the common slots for Actors (slots 1 and 3) so we can
     * reference them all at one point. I'm envisioning:
     *
     * On creation of Envelope:
     *      actor_ref_then_push(L, 1);
     *
     * For getting info out:
     *      actor_deref_then_push(L, 1);
     *
     *      Each action is ran under the conditions that no actor referencing 
     * is done outside the set slots. This allows references to disappear 
     * introducing the notion of checking nil. Using actor:ref assures that 
     * the reference will keep the actor from being garbage collected.
     *
     *
     *      After a few moments, I had the thought that if I just make 
     * actor:audience use actor:ref then, all the references would be made 
     * automatically for us. And on second thought -- this puts us in Lua 
     * territory. We should only be concerned for the references that exist in
     * our system.
     *
     *      We need to be concerned with direct references inside our Sytem and 
     * direct references outside the system to actors inside the system. So, 
     * the interpreter land should count as references and should keep an actor
     * from being deleted (and garbage collected), but not benched.
     */
    lua_newtable(L);
    for (i = 1; i <= args; i++) {
        lua_pushvalue(L, i);
        lua_rawseti(L, -2, i);
    }

    for (i = 0; i < post->postmen_count; i = (i + 1) % post->postmen_count)
        if (mailbox_send_lua_top(post->postmen[i]->mailbox, L))
            break;

    return 1;
}

int
lua_post_gc (lua_State *L)
{
    Post *post = lua_check_post(L, 1);
    int i;

    for (i = 0; i < post->postmen_count; i++)
        postman_stop(post->postmen[i]);

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
