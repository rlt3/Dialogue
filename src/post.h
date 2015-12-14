#ifndef DIALOGUE_POST
#define DIALOGUE_POST

#define POST_LIB "Dialogue.Post"

#include "actor.h"
#include "mailbox.h"

/*
 * Every Postman has its own thread and Lua state. It delivers all the 
 * Envelopes in its state. It then waits and its state fills up with 
 * Envelopes and then it is told to process them.
 */
typedef struct Postman {
    lua_State *L;
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t work_cond;
    Actor *author;
    const char *tone;
    short int working;
} Postman;

/*
 * The Post is a collection of Postmen. It provides a way to quickly send
 * Envelopes to a waiting Postman.
 */
typedef struct Post {
    Postman **postmen;
    int postmen_count;
} Post;

/*
 * Expects a message on top of the Actor's Lua stack. The Post finds a free
 * Postman and creates an Envelope in the Postman's state and the Postman
 * delivers it.
 */
void
post_deliver_lua_top (Post *post, Actor *author, const char *tone);

int 
luaopen_Dialogue_Post (lua_State *L);

#endif
