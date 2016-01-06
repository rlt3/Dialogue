#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "interpreter.h"

#define LINE_SIZE 256

static pthread_t thread;
static pthread_mutex_t mutex;
static pthread_cond_t wait_cond;
static short int input_ready;
static short int *running;
static const char *input_line;

int interpreter_next_input ();

/*
 * Interpret the input in the given lua_State*.
 */
void
lua_interpret (lua_State *L)
{
    const char *input = interpreter_poll_input();

    if (input == NULL)
        return;

    if (luaL_loadstring(L, input) || lua_pcall(L, 0, 0, 0))
        printf("%s\n", lua_tostring(L, -1));
}

/*
 * Exit the interpreter.
 */
void
interpreter_exit ()
{
    puts("\nGoodbye.");
    *running = 0;
}

void
interpreter_cancel ()
{
    interpreter_exit();
    pthread_kill(thread, SIGINT);
}

/*
 * Exit the interpreter from inside the interpreter through 'exit()'
 */
int
lua_interpreter_exit (lua_State *L)
{
    interpreter_exit();
    return 0;
}

/*
 * Exit the interpreter via CTRL-C
 */
void 
handle_sig_int (int arg)
{
    interpreter_exit();
}

/*
 * Polls the interpreter to see if it has any input. If there's no input 
 * available it returns NULL. Returns the string if there is.
 */
const char *
interpreter_poll_input ()
{
    const char *ret = NULL;
    int rc = pthread_mutex_trylock(&mutex);

    if (rc == EBUSY)
        return NULL;

    if (input_ready)
        ret = input_line;

    input_ready = 0;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&wait_cond);
    return ret;
}

/*
 * Wait for input to be received. When it is, unlock the mutex and wait for the
 * signal to start looking for more input.
 */
void *
interpreter_thread (void *arg)
{
    char *line;
    signal(SIGINT, handle_sig_int);
    pthread_mutex_lock(&mutex);

    printf("Dialogue v0.0 with Lua v5.2\n"
           "    type `exit()` to exit.\n");

    while (*running) {
        printf("waiting");
        line = readline("> ");
        add_history(line);

        //if (!line)
        //    continue;

        input_line = line;
        input_ready = 1;
        while (input_ready)
            pthread_cond_wait(&wait_cond, &mutex);
        input_line = NULL;
        free(line);
    }

    printf("quiting");
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/*
 * Register a given Lua state with the interpreter. This sets an exit function
 * to that Lua state using the is_running_ptr, which is a pointer to a boolean
 * integer.
 */
int
interpreter_register (lua_State *L, short int *is_running_ptr)
{
    pthread_mutexattr_t mutex_attr;

    running = is_running_ptr;
    input_line = NULL;
    wait_cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &mutex_attr);
    
    lua_pushcfunction(L, lua_interpreter_exit);
    lua_setglobal(L, "exit");

    pthread_create(&thread, NULL, interpreter_thread, NULL);

    return 0;
}
