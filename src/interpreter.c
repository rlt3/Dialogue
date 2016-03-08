#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "interpreter.h"

static pthread_t global_thread;
static pthread_mutex_t global_mutex;
static pthread_cond_t global_cond;
static char *global_line = NULL;
static int global_running = 1;
static int global_get_input = 1;

void
interpreter_addline (char *line)
{
    global_get_input = 0;
    global_line = line;
    add_history(global_line);
}

/*
 * The thread uses Readline's async functions so we can interrupt getting the 
 * next input char and safely exit the thread any time.
 */
void *
interpreter_thread (void *arg)
{
    pthread_mutex_lock(&global_mutex);
    rl_callback_handler_install("> ", interpreter_addline);

    printf("Dialogue v0.0 with Lua v5.2\n"
           "    type `exit()` to exit.\n");

    while (global_running) {
        /*
         * when the `rl_callback_read_char` gets to the end of the line, it
         * calls `interpreter_addline` which sets `global_get_input` to false.
         */
        while (global_get_input)
            rl_callback_read_char();

        while (global_running && global_line != NULL)
            pthread_cond_wait(&global_cond, &global_mutex);
    }

    rl_callback_handler_remove();
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

    *input = global_line;
    global_line = NULL;
    global_get_input = 1;
    pthread_cond_signal(&global_cond);
    pthread_mutex_unlock(&global_mutex);

    ret = 0;
exit:
    return ret;
}

void
interpreter_destroy ()
{
    global_get_input = 1;
    global_running = 0;
    pthread_join(global_thread, NULL);
    puts("\nGoodbye.");
}
