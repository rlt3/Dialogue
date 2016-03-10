#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "console.h"

static pthread_t cons_thread;
static pthread_mutex_t cons_mutex;
static pthread_cond_t cons_cond;

static char *cons_prev_input;
static char *cons_input;

static pthread_mutex_t cons_running_mutex;
static volatile int cons_is_running;

static const char *prompt = "> ";

int
lua_console_log (lua_State *L)
{
    console_log(luaL_checkstring(L, 1));
    return 0;
}

/*
 * Override the `io.write` method to one that can handle our console.
 */
void
console_set_write (lua_State *L)
{
    lua_getglobal(L, "io");
    lua_pushcfunction(L, lua_console_log);
    lua_setfield(L, -2, "write");
    lua_pop(L, 1);
}

/*
 * Handle sigint.
 * TODO: I'm not even sure this works. Also handle ctrl+d as well as ctrl+c.
 */
void
console_handle_interrupt (int arg)
{
    console_log("To quit type `exit`!\n");
}

/*
 * The thread uses Readline's async functions so we can interrupt getting the 
 * next input char and safely exit the thread any time.
 */
void *
console_thread (void *arg)
{
    printf("Dialogue v%s with Lua v%s\n"
           "↳  type `exit` to quit.\n", DIALOGUE_VERSION,DIALOGUE_LUA_VERSION);

    pthread_mutex_lock(&cons_mutex);
    
    while (1) {
        cons_input = readline(prompt);

        if (strncmp("exit", cons_input, 4) == 0) {
            /* free and set NULL here so input isn't caught before quitting */
            free(cons_input);
            cons_input = NULL;
            break;
        }

        add_history(cons_input);

        /* `console_poll_input` resets the condition */
        while (cons_input != NULL)
            pthread_cond_wait(&cons_cond, &cons_mutex);
    }

    pthread_mutex_unlock(&cons_mutex);

    pthread_mutex_lock(&cons_running_mutex);
    cons_is_running = 0;
    pthread_mutex_unlock(&cons_running_mutex);

    return NULL;
}

/*
 * Load the console thread.
 * Return 0 if successful, otherwise an error.
 */
int
console_create ()
{
    int ret = 1;

    pthread_mutex_init(&cons_mutex, NULL);
    pthread_cond_init(&cons_cond, NULL);

    if (pthread_create(&cons_thread, NULL, console_thread, NULL) != 0)
        goto exit;

    cons_is_running = 1;
    cons_prev_input = NULL;
    cons_input = NULL;

    ret = 0;
exit:
    return ret;
}

/*
 * Returns 1 or 0 (true or false) whether or not the console is still 
 * running.
 */
int
console_is_running ()
{
    int running = 0;
    pthread_mutex_lock(&cons_running_mutex);
    running = cons_is_running;
    pthread_mutex_unlock(&cons_running_mutex);
    return running;
}

/*
 * Log the formart string to the console. Returns the number of characters 
 * printed.
 */
int 
console_log (const char *fmt, ...)
{
    /*
     * TODO: Everything about outputting to the console asynchronously.
     *  Why does the console "redisplay" the last line sometimes? The
     *  occurance bellow happens far too often.
     *
     *  > ffffff<enter>
     *  > ffffff> ffffff<enter>
     *  > ><enter>
     *  >
     */
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
    va_list args;
    char* saved_line;
    int saved_point;

    pthread_mutex_lock(&log_mutex);
    saved_point = rl_point;
    saved_line = rl_copy_text(0, rl_end);
    rl_set_prompt("");
    rl_replace_line("", 0);
    rl_redisplay();
    
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    rl_set_prompt(prompt);
    rl_replace_line(saved_line, 0);
    rl_point = saved_point;
    rl_redisplay();
    free(saved_line);
    pthread_mutex_unlock(&log_mutex);

    return 0;
}

/*
 * Poll console for input. If it returns 0, the value at the `input` pointer
 * is set to the input string. Else, the `input` pointer is set to NULL.
 */
int
console_poll_input (char **input)
{
    int ret = 1;
    int rc = pthread_mutex_trylock(&cons_mutex);

    if (rc != 0)
        goto exit;

    if (!cons_input)
        goto cleanup;

    free(cons_prev_input);
    *input = cons_input;
    cons_prev_input = cons_input;
    cons_input = NULL;
    pthread_cond_signal(&cons_cond);

    ret = 0;
cleanup:
    pthread_mutex_unlock(&cons_mutex);
exit:
    return ret;
}

void
console_cleanup ()
{
    pthread_join(cons_thread, NULL);

    free(cons_prev_input);
    cons_prev_input = NULL;

    puts("Goodbye.");
}
