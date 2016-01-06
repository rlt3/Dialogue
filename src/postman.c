#include <stdlib.h>
#include "postman.h"
#include "post.h"
#include "dialogue.h"
#include "mailbox.h"
#include "action.h"
#include "luaf.h"
#include "utils.h"

static const char *postman_bag = "__postman_bag";
//static const int resend_arg = 1;
static const int bag_arg = 2;

/*
 * All functions inside the Postman thread expect the above constants to be
 * true when it is called.
 *
 * This handles each action of Dialogue.
 */
void *
postman_thread (void *arg)
{
    Postman *postman = arg;
    const char *action;
    lua_State *P = postman->L;

    while (postman->working) {
        /* Create the 'resend bag' and push the 'postman bag' */
        lua_newtable(P);
        lua_getglobal(P, postman_bag);

        /* fill the postman's bag with any envelopes from its mailbox */
        postman_fill_bag(postman);

        /*
         * Anyone of these actions may add an envelope from the 'postman bag'
         * to the 'resend bag'. This is so we can keep Postman always working
         * instead of waiting on a resource.
         */
        lua_pushnil(P);
        while (lua_next(P, bag_arg)) {

            /*
             * TODO:
             *    Handle actor and action requirement as separate variables
             * and have the 'send' message table be an actual optional table
             * so that this can move more fluidly.
             */

            lua_rawgeti(P, -1, 2);
            action = lua_tostring(P, -1);
            lua_pop(P, 1);

            switch (action[0]) {
                case 'c': /* create */
                    action_create(P);
                    break;

                case 'b': /* bench  */
                    action_bench(P);
                    break;

                case 'j': /* join   */
                    action_join(P);
                    break;

                case 'r': /* remove */
                    action_remove(P);
                    break;

                case 'd': /* delete */
                    action_delete(P);
                    break;

                case 'l': /* load   */
                    action_load(P);
                    break;

                case 's': /* send   */
                    action_send(P);
                    break;

                case 'e': /* error  */
                    action_error(P);
                    break;

                default:
                    luaf(P, "Dialogue.Post.send(nil, 'error', 'Nil Action!')");
                    break;
            }
            postman->messages_processed++;
            lua_pop(P, 1);
        }

        /* Set the resend bag as our 'postman bag' */
        lua_pop(P, 1);
        lua_setglobal(P, postman_bag);
    }

    return NULL;
}

/*
 * If the postman's bag still has envelopes, do nothing. Otherwise, wait for 
 * the mailbox to be free and then fill the bag with envelopes from the mailbox.
 */
void
postman_fill_bag (Postman *postman)
{
    lua_State *P = postman->L;

    /* 
     * TODO:
     *  It seems I *have* to run lua_type for luaL_len to work here. I don't
     *  know why. Figure it out.
     */
    if (lua_type(P, bag_arg) == LUA_TTABLE)
        if (luaL_len(P, bag_arg) > 0)
            return;

    lua_pop(P, 1);
    mailbox_pop_envelopes(postman->mailbox, P);
    lua_setglobal(P, postman_bag);
    lua_getglobal(P, postman_bag);
}

Postman *
postman_create (void *post)
{
    lua_State *P;
    Postman *postman = malloc(sizeof(*postman));

    postman->working = 1;
    postman->messages_processed = 0;
    postman->mailbox = mailbox_create();
    postman->L = luaL_newstate();
    P = postman->L;
    luaL_openlibs(P);
    
    /* 
     * TODO:
     *      load Dialogue's core modules somehow instead of reloading the 
     * entire Dialogue module. this creates ambiguity with the Post & Postman
     * threads.
     */
    luaL_requiref(P, "Dialogue", luaopen_Dialogue, 1);
    lua_getfield(P, -1, "Post");
    utils_push_object(P, post, POST_LIB);
    lua_setfield(P, -2, "__obj");
    lua_pop(P, 2);

    pthread_create(&postman->thread, NULL, postman_thread, postman);

    /*
     * semaphore for the per_thread?
     */
    return postman;
}

void
postman_stop (Postman *postman)
{
    printf("%p processed: %ld\n", postman, postman->messages_processed);
    postman->working = 0;
    pthread_join(postman->thread, NULL);
    free(postman);
}
