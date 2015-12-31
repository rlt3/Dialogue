#include "postman.h"
#include "actions.h"

static const char *postman_bag = "__postman_bag";
static const int resend_arg = 1;
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
        while (lua_next(P, bag_arg)) {
            switch (envelope->action) {
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
            lua_pop(P, 1);
        }

        /* Set the resend bag as our 'postman bag' */
        lua_pop(L, 1);
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

    if (luaL_len(P, bag_arg) > 0)
        return;

    lua_pop(P, 1);
    mailbox_pop_envelopes(postman->mailbox, P);
    lua_setglobal(P, postman_bag);
}

Postman *
postman_create ()
{
    Postman *postman = malloc(sizeof(*postman));
    postman->mailbox = mailbox_create();

    /*
     * semaphore for the per_thread?
     */
    return postman;
}
