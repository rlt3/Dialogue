#ifndef DIALOGUE_CONSOLE
#define DIALOGUE_CONSOLE

#include "dialogue.h"

#define CONSOLE_THREADED 1
#define CONSOLE_NON_THREADED 0

/*
 * Override the `io.write` method to one that can handle our console.
 */
void
console_set_write (lua_State *L);

/*
 * Log the formart string to the console.
 */
void
console_log (const char *fmt, ...);

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
console_start (lua_State *L, const char *file, const int is_threaded);

/*
 * Only needs to be called if CONSOLE_THREADED was passed into `console_start`.
 */
void
wait_for_console_exit ();

#endif
