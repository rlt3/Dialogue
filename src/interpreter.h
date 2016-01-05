#ifndef DIALOGUE_INTERPRETER
#define DIALOGUE_INTERPRETER

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct Interpreter Interpreter;

/*
 * Poll for any input and, if there is, interpret it.
 */
void
lua_interpret (lua_State *L);

/*
 * If there is input ready, returns it. Else NULL.
 */
const char *
interpreter_current_input ();

/*
 * If the interpreter has stopped to give input, start it again. 
 * Returns 1 if busy, 0 if started again.
 */
int
interpreter_next_input ();

/*
 * Polls the interpreter to see if it has any input. If there's no input 
 * available it returns 0. If there is, returns 1.
 */
int
interpreter_poll (Interpreter *interpreter);

/*
 * Register a given Lua state with the interpreter. This sets an exit function
 * to that Lua state using the is_running_ptr, which is a pointer to a boolean
 * integer.
 */
int
interpreter_register (lua_State *L, short int *is_running_ptr);

#endif
