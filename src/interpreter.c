#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "interpreter.h"

static pthread_t global_thread;
static pthread_mutex_t global_mutex;
static pthread_cond_t global_cond;
static char *global_line = NULL;
static volatile int global_running = 1;

/*
 * The thread uses Readline's async functions so we can interrupt getting the 
 * next input char and safely exit the thread any time.
 */
void *
interpreter_thread (void *arg)
{
    printf("Dialogue v0.0 with Lua v5.2\n"
           "    type `exit()` to exit.\n");

    pthread_mutex_lock(&global_mutex);
    
    while (global_running) {
        while (global_running && global_line != NULL)
            pthread_cond_wait(&global_cond, &global_mutex);

        global_line = readline("> ");
        add_history(global_line);
    }

    pthread_mutex_unlock(&global_mutex);

    return NULL;
}

/*
 * Load the interpreter thread.
 * Return 0 if successful, otherwise an error.
 */
int
interpreter_create ()
{
    int ret = 1;

    pthread_mutex_init(&global_mutex, NULL);
    pthread_cond_init(&global_cond, NULL);

    if (pthread_create(&global_thread, NULL, interpreter_thread, NULL) != 0)
        goto exit;

    ret = 0;
exit:
    return ret;
}

/*
 * Poll interpreter for input. If it returns 0, the value at the `input` pointer
 * is set to the input string. Else, the `input` pointer is set to NULL.
 *
 * if (interpreter_poll_input(&input) == 0)
 */
int
interpreter_poll_input (char **input)
{
    int ret = 1;
    int rc = pthread_mutex_trylock(&global_mutex);

    if (rc != 0)
        goto exit;

    if (!global_line)
        goto cleanup;

    *input = global_line;
    global_line = NULL;
    pthread_cond_signal(&global_cond);

    ret = 0;
cleanup:
    pthread_mutex_unlock(&global_mutex);
exit:
    return ret;
}

void
interpreter_destroy ()
{
    /*
     * A filthy hack I found on StackOverflow:
     *      http://stackoverflow.com/a/2792129/5243467
     *
     * Since we need the main thread to handle Actor messages, we put the IO in
     * a separate thread. Readline wasn't really meant for threads. So, when 
     * you need to exit, the readline will wait in a loop until *some* input is
     * entered (so it exits the loop and checks if its still running). Setting
     * `rl_getc_function` allows graceful signaling, returning NULL.
     */

    global_running = 0;
    rl_getc_function = getc;
    pthread_kill(global_thread, SIGINT);
    //pthread_join(global_thread, NULL);
    puts("\nGoodbye.");
}
