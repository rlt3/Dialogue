#ifndef DIALOGUE_POST
#define DIALOGUE_POST

#define POST_LIB "Dialogue.Post"

#include "actor.h"
#include "mailbox.h"

/*
 * Every Postman has its own thread and Lua state and also a Mailbox which has
 * its own Lua state. While sending messages, it looks to see if any envelopes
 * are in its mailbox. If so, it delivers them all.
 * 
 * While delivering, the Postman's mailbox can receive messages. While sending
 * messages, if a message can't be delivered *right then*, it is skipped and
 * left at the top of the queue of the Postman's 'bag'. When the Postman goes
 * to refill his bag at the end of the Envelopes, it adds the new ones beneath
 * the skipped envelopes.
 */
typedef struct Postman {
    lua_State *L;

    pthread_t thread;
    short int working;

    pthread_mutex_t lock;
    pthread_cond_t wait_cond;
    short int waiting;

    struct Mailbox *mailbox;
} Postman;

/*
 * The Post is a collection of Postmen & Mailboxes. It provides a way to
 * quickly send Envelopes to a waiting Postman.
 */
typedef struct Post {
    Postman **postmen;
    int postmen_count;
    int ref;
} Post;

Post *
lua_check_post (lua_State *L, int index);

/*
 * Expects a message on top of the given Lua state. The Post finds a free
 * Postman, copies the message from the Lua stack to the Postman's stack.  Then
 * it delivers it to the correct audience, given by the tone. Pops the message
 * on top.
 */
void
post_deliver_lua_top (lua_State *L, Post *post, Actor *author, const char *tone);

int 
luaopen_Dialogue_Post (lua_State *L);

#endif
