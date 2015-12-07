#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define SCRIPT_LIB "Dialogue.Actor.Script"

#define ERR_NOT_CALLING_THREAD "Calling thread isn't the Actor's thread."
#define ERR_BAD_MODULE         "The Script's module isn't valid or has errors."
#define ERR_NO_MODULE_NEW      "'new' doesn't exist for the module."
#define ERR_BAD_MODULE_NEW     "'new' failed with given arguments."

#define LOAD_OK         0
#define LOAD_BAD_THREAD 1
#define LOAD_FAIL       2

#define SEND_OK         0
#define SEND_BAD_THREAD 1
#define SEND_SKIP       2
#define SEND_FAIL       3

typedef struct Script {
    struct Script *next;
    struct Actor *actor;

    short int is_loaded;  /* is it loaded? true/false */
    short int be_loaded;  /* should it be loaded? true/false */

    const char *error; /* if not loaded & shouldn't be loaded, here's why */

    int table_ref;  /* the definition of the script for reloading */
    int object_ref; /* the object that resides in the Actor's stack */
    int ref;        /* a place for the actor to hold the ref so its not gc'd */
} Script;

/*
 * Assumes the state mutex has been acquired.  Attempt to load the given
 * Script. *Must* be called from the Actor's thread.
 *
 * Returns: LOAD_OK, LOAD_BAD_THREAD, LOAD_FAIL
 *
 * If LOAD_FAIL is returned, the Script is turned off and exists as dead weight
 * -- skipped by all messages and sends the error which caused it not to load
 *  on any access of it. A user can to attempt to fix the errors and reload it
 *  manually.
 *
 * Additionally, details of the error are set if LOAD_FAIL is returned.
 */
int
script_load (Script *script);


/*
 * Assumes the state mutex has been acquired and there is a message at the top
 * of the stack.
 *
 * Returns SEND_OK, SEND_BAD_THREAD, SEND_SKIP, SEND_FAIL.
 *
 * Details of the error are set if SEND_FAIL is returned.
 */
int
script_send (Script *script);

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index);

int 
luaopen_Dialogue_Actor_Script (lua_State *L);

#endif
