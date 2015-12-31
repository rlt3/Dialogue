#ifndef DIALOGUE_POST_POSTMAN
#define DIALOGUE_POST_POSTMAN

#include "mailbox.h"
#include <pthread.h>

typedef struct Postman {
    lua_State *L;
    pthread_t thread;
    int working;
    struct Mailbox *mailbox;
} Postman;

#endif
