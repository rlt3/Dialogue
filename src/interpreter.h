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
 * Polls the interpreter to see if it has any input. If there's no input 
 * available it returns NULL. Returns the string if there is.
 */
const char *
interpreter_poll_input ();

/*
 * Register a given Lua state with the interpreter. This sets an exit function
 * to that Lua state using the is_running_ptr, which is a pointer to a boolean
 * integer.
 */
int
interpreter_register (lua_State *L, short int *is_running_ptr);

#endif
