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

static const char *console_file = NULL;
static const char *console_prompt = "> ";
static pthread_t console_pthread;
static pthread_mutex_t exit_mutex = PTHREAD_MUTEX_INITIALIZER;
static lua_State *C = NULL;
static int running = 1;  

void
console_exit ()
{
    pthread_mutex_lock(&exit_mutex);
    running = 0;
    pthread_mutex_unlock(&exit_mutex);
}

int
console_is_running ()
{
    int ret = 0;
    pthread_mutex_lock(&exit_mutex);
    ret = running;
    pthread_mutex_unlock(&exit_mutex);
    return ret;
}

void 
input_rlhandler (char *line)
{
    if (line == NULL) {
        /* readline returns NULL on a signal interrupt */
        console_exit();
    } else {
        if (*line != 0) {
            add_history(line);

            if (luaL_loadstring(C, line) || lua_pcall(C, 0, 0, 0)) {
                console_log("%s\n", lua_tostring(C, -1));
                lua_pop(C, 1);
            }
        }

        free(line);
    }
}

int
lua_console_exit (lua_State *L)
{
    console_exit();
    return 0;
}

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

    //lua_pushcfunction(L, lua_console_exit);
    //lua_setglobal(L, "exit");
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

void*
console_thread (void *arg)
{
    C = arg;
    //char *input = NULL;

    printf("Dialogue v%s with Lua v%s\n"
           "↳  type `exit` to quit.\n", 
           DIALOGUE_VERSION, DIALOGUE_LUA_VERSION);

    //signal(SIGINT, console_handle_interrupt);
    console_set_write(C);

    lua_pushcfunction(C, lua_console_exit);
    lua_setglobal(C, "exit");

    if (luaL_loadfile(C, console_file) || lua_pcall(C, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", 
                console_file, lua_tostring(C, -1));
        goto exit;
    }

    rl_callback_handler_install(console_prompt, input_rlhandler);

    while (console_is_running())
        rl_callback_read_char();

    //while (1) {
    //    input = readline(console_prompt);

    //    if (strncmp("exit", input, 4) == 0)
    //        goto exit;

    //    if (strlen(input) > 0)
    //        add_history(input);

    //    if (luaL_loadstring(L, input) || lua_pcall(L, 0, 0, 0)) {
    //        console_log("%s\n", lua_tostring(L, -1));
    //        lua_pop(L, 1);
    //    }

    //    free(input);
    //}

    rl_callback_handler_remove();

exit:
    //free(input);
    lua_close(C);
    console_file = NULL;
    puts("Goodbye.");

    return NULL;
}

/*
 * Log the formart string to the console.
 */
void
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

    rl_set_prompt(console_prompt);
    rl_replace_line(saved_line, 0);
    rl_point = saved_point;
    rl_redisplay();
    free(saved_line);
    pthread_mutex_unlock(&log_mutex);
}

/*
 * The console takes ownership of the given Lua state and acts as an interpreter
 * for it. It loads the given `stage` file. The console controls the exit for
 * the entire program and will cleanup the Lua state itself.
 *
 * If is_threaded is true then the console is started in a new thread
 * and this function can return 1 if creating the thread fails. If is_threaded
 * is false the function blocks, runs the console, and stops blocking when the
 * user has exited it.
 *
 * Returns 0 when successful.
 */
int
console_start (lua_State *L, const char *file, const int is_threaded)
{
    int ret = 1;

    console_file = file;

    if (is_threaded) {
        if (pthread_create(&console_pthread, NULL, console_thread, L) != 0) {
            lua_close(L);
            goto exit;
        }
    } else {
        console_thread(L);
    }

    ret = 0;
exit:
    return ret;
}

void
wait_for_console_exit ()
{
    pthread_join(console_pthread, NULL);
}
