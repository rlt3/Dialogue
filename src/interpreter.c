#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "interpreter.h"

#define LINE_SIZE 256

struct Interpreter {
    pthread_t thread;
    pthread_mutex_t input_mutex;
    pthread_cond_t wait_cond;
    short int waiting;
    short int *is_running;
    const char *line;
};

/*
 * Print the Lua error.
 */
void
lua_printerror (lua_State *L)
{
    printf("%s\n", lua_tostring(L, -1));
}

/*
 * Interpret the input in the given lua_State*.
 */
void
lua_interpret (lua_State *L, const char *input)
{
    if (input == NULL)
        return;

    lua_getglobal(L, "loadstring");
    lua_pushstring(L, input);
    lua_call(L, 1, 1);
    
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0))
            lua_printerror(L);
    } else {
        lua_pop(L, 1);
    }
}

/*
 * Wait for input to be received. When it is, unlock the mutex and wait for the
 * signal to start looking for more input.
 */
void *
interpreter_thread (void *arg)
{
    char *input;
    Interpreter *interpreter = arg;

    pthread_mutex_lock(&interpreter->input_mutex);

    printf("Dialogue v0.0 with Lua v5.2\n"
           "    type `exit()` to exit.\n");

    while (*interpreter->is_running) {

        input = readline("> ");
        add_history(input);

        interpreter->line = input;
        interpreter->waiting = 1;

        while (interpreter->waiting) {
            pthread_cond_wait(&interpreter->wait_cond, 
                    &interpreter->input_mutex);
        }
    }

    printf("Goodbye.\n");
    free(interpreter);

    return NULL;
}

/*
 * Exit the interpreter safely from within Lua.
 */
int 
lua_exit (lua_State *L)
{
    Interpreter *interpreter;
    lua_getglobal(L, "__interpreter");

    interpreter = lua_touserdata(L, 1);
    *interpreter->is_running = 0;

    return 0;
}

/*
 * Evaluate the string given as Lua code.
 */
int 
lua_eval (lua_State *L)
{
    const char *string = luaL_checkstring(L, 1);

    if (luaL_loadstring(L, string) || lua_pcall(L, 0, 0, 0))
        lua_printerror(L);

    return 0;
}

/*
 * Polls the interpreter to see if it has any input. If there's no input 
 * available it returns 0. If there is, returns 1.
 */
int
interpreter_poll (Interpreter *interpreter)
{
    int rc = pthread_mutex_trylock(&interpreter->input_mutex);

    if (rc == EBUSY)
        return 0;

    pthread_mutex_unlock(&interpreter->input_mutex);
    return 1;
}

/*
 * When the interpreter has input, this function uses that available input and
 * parses and executes the input for the given lua_State.
 */
void
interpreter_lua_interpret (Interpreter *interpreter, lua_State *L)
{
    pthread_mutex_lock(&interpreter->input_mutex);

    lua_interpret(L, interpreter->line);
    interpreter->line = NULL;
    interpreter->waiting = 0;

    pthread_mutex_unlock(&interpreter->input_mutex);
    pthread_cond_signal(&interpreter->wait_cond);
}

/*
 * Create an interpreter for the given lua_State. Pass in a pointer to a 
 * boolean so the interpreter can be directly tied to the main thread and quit
 * the system.
 */
Interpreter *
interpreter_create (lua_State *L, short int *is_running_ptr)
{
    pthread_mutexattr_t mutex_attr;
    Interpreter *interpreter = malloc(sizeof(*interpreter));

    if (interpreter == NULL)
        goto exit;

    interpreter->is_running = is_running_ptr;
    interpreter->line = NULL;
    interpreter->waiting = 0;
    interpreter->wait_cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutex_init(&interpreter->input_mutex, &mutex_attr);

    lua_pushlightuserdata(L, interpreter);
    lua_setglobal(L, "__interpreter");
    
    lua_pushcfunction(L, lua_eval);
    lua_setglobal(L, "eval");  
    
    lua_pushcfunction(L, lua_exit);
    lua_setglobal(L, "exit");  

    pthread_create(&interpreter->thread, NULL, interpreter_thread, interpreter);
    pthread_detach(interpreter->thread);
exit:
    return interpreter;
}
