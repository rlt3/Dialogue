#ifndef DIALOGUE_CONSOLE
#define DIALOGUE_CONSOLE

/*
 * Load the console thread.
 * Return 0 if successful, otherwise an error.
 */
int
console_create ();

/*
 * Handle sigint.
 */
void
console_handle_interrupt (int arg);

/*
 * Returns 1 or 0 (true or false) whether or not the console is still 
 * running.
 */
int
console_is_running ();

/*
 * Poll console for input. If it returns 0, the value at the `input` pointer
 * is set to the input string. Else, the `input` pointer is set to NULL.
 *
 * if (console_poll_input(&input) == 0)
 */
int
console_poll_input (char **input);

void
console_cleanup ();

#endif
