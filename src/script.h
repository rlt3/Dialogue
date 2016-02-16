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
 * Loads the Script created in the given Lua stack. 
 * Returns 0 if successful, 1 if an error occurs. If an error occurs, an error
 * string is pushed onto A.
 */
int
script_load (Script *script, lua_State *A);

/*
 * Assumes the Lua stack A has a message table on top in the form of:
 *  { 'message' [, arg1 [, ... [, argn]]], author}
 *
 * Returns 0 if successful, 1 if an error occurs. If an error occurs, an error
 * string is pushed onto A.
 */
int
script_send (Script *script, lua_State *A);

void
script_destroy (Script *script, lua_State *A);

/*
 * To avoid longjmps from the Actors' Lua stacks, we pop any error message off 
 * them and onto the global (or calling) Lua state and error from there.
 */
void
script_error (lua_State *L, lua_State *A);

#endif
