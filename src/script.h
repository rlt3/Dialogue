#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include "actor.h"

typedef struct Script {
    struct Script *prev;
    struct Script *next;

    short int is_loaded;  /* is it loaded? true/false */
    short int be_loaded;  /* should it be loaded? true/false */

    int table_ref;  /* the definition of the script for reloading */
    int object_ref; /* the object that resides in the Actor's stack */
} Script;

/*
 * Expects a Script definition on top of the Lua stack A.  Returns a pointer to
 * the Script.
 *
 * If the functions returns NULL, an error string is pushed onto A. Otherwise
 * nothing is pushed onto A and the function returns the Script.
 */
Script *
script_new (lua_State *A);

/*
 * Loads (or reloads) the Script created in the given Lua stack.  
 *
 * Assume a Script definition table at -1 in the form of
 *  { 'module' [, arg1 [, ... [, argn]]] }
 *
 * The module is `required' and then the function `new` is called on the table
 * that is returned from `require`. The args supplied in the script definition
 * are passed into the `new` function.
 *
 * Returns 0 if successful, 1 if an error occurs. If an error occurs, an error
 * string is pushed onto A.
 */
int
script_load (Script *script, lua_State *A);

/*
 * Sends a Message to the object created from script_load.
 *
 * Assumes a message definition table at -1 in the form of
 *  { 'message' [, arg1 [, ... [, argn]]], author}
 *
 * The 'message' is some method of the instantiated object which was created in
 * script_load. If it doesn't exist it actually isn't an error. This is one of
 * those "it's a feature, not a bug" things -- by willing to say this isn't an
 * error we gain the ability to very easily add new message primitives (the
 * methods themselves) to the system.
 *
 * Returns 0 if successful, 1 if an error occurs. If an error occurs, an error
 * string is pushed onto A.
 */
int
script_send (Script *script, lua_State *A);

/*
 * Access a field and get the results from the object inside the Script.
 *
 * If there is an error this function returns 1 and leaves an error string on
 * top of the Actor's stack. Returns 0 if successful and leaves the probed
 * value on top of the Actor's stack.
 */
int
script_probe (Script *script, lua_State *A, const char *field);

void
script_destroy (Script *script, lua_State *A);

#endif
