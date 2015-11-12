#ifndef POSTMAN_H
#define POSTMAN_H

typedef struct Postman {
    pthread_mutex_t mutex;
    pthread_t thread;
} Postman;

#endif
