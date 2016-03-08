#ifndef DIALOGUE_INTERPRETER
#define DIALOGUE_INTERPRETER

/*
 * Load the interpreter thread.
 * Return 0 if successful, otherwise an error.
 */
int
interpreter_create ();

/*
 * Poll interpreter for input. If it returns 0, the value at the `input` pointer
 * is set to the input string. Else, the `input` pointer is set to NULL.
 *
 * if (interpreter_poll_input(&input) == 0)
 */
int
interpreter_poll_input (char **input);

void
interpreter_destroy ();

#endif
