#ifndef DIALOGUE_POST
#define DIALOGUE_POST

#define POST_LIB "Dialogue.Post"

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
    const char *author;
    const char *tone;
} Postman;

/*
 * The Post is a collection of Postmen. It provides a way to quickly send
 * Envelopes to a waiting Postman.
 */
typedef struct Post {
    Postman **postmen;
    int postmen_count;
} Post;

#endif
