#ifndef DIALOGUE
#define DIALOGUE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef DIALOGUE_MODULE
#define console_log printf
#endif

#define DIALOGUE_VERSION     "0.0"
#define DIALOGUE_LUA_VERSION "5.2"

enum DialogueOption {
    WORKER_IS_MAIN, WORKER_COUNT, ACTOR_COUNT, ACTOR_CHILD_MAX,
    ACTOR_FORCE_SYNC, ACTOR_CONSOLE_WRITE, ACTOR_MANUAL_LOAD
};

/*
 * Set the correct io.write function depending on if we're outputting to the 
 * console or just stdout.
 */
void
dialogue_set_io_write (lua_State *L);

/*
 * A truth function for whether or not we are forcing asynchronous calls to be
 * ran synchronously.
 */
int
dialogue_forced_synchronous ();

/*
 * A truth function for whether or not the Actors are loaded manually or not.
 */
int
dialogue_actor_manual_load ();

void
dialogue_option_set (enum DialogueOption option, int value);

int
luaopen_Dialogue (lua_State *L);

#endif
