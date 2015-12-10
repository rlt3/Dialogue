#ifndef DIALOGUE_INTERPRETER
#define DIALOGUE_INTERPRETER

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct Interpreter Interpreter;

/*
 * Interpret the input in the given lua_State*.
 */
void
lua_interpret (lua_State *L, const char *input);

/*
 * Create an interpreter for the given lua_State. Pass in a pointer to a 
 * boolean so the interpreter can be directly tied to the main thread and quit
 * the system.
 */
Interpreter *
interpreter_create (lua_State *L, short int *done_ptr);

/*
 * Polls the interpreter to see if it has any input. If there's no input 
 * available it returns 0. If there is, returns 1.
 */
int
interpreter_poll (Interpreter *interpreter);

/*
 * When the interpreter has input, this function uses that available input and
 * parses and executes the input for the given lua_State.
 */
void
interpreter_lua_interpret (Interpreter *interpreter, lua_State *L);

#endif
