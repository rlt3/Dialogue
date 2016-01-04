#ifndef DIALOGUE_ACTION
#define DIALOGUE_ACTION

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/*
 * These are the Actions of Dialogue -- all the primitives of the system.
 */

enum Action {
    CREATE,
    REMOVE,
    DELETE,
    LOAD,
    SEND,
    RECEIVE,
    END
};

/*
 * Create an Actor as an explicit child of another Actor (not the actual 
 * creation of the original Dialogue tree). 
 * Expects the Author on top and the definition table beneath.
 */
int
action_create (lua_State *L);

/*
 */
int
action_bench (lua_State *L);

/*
 */
int
action_join (lua_State *L);

/*
 * Attempt to remove and quarantine an Actor from the Dialogue tre before
 * DELETEing it. While quarantined its reference is still valid, but all 
 * traffic via the tree will cease.
 */
int
action_remove (lua_State *L);

/*
 * When a REMOVE action is called, if it succeeds, it sends a DELETE message.
 * Every time the DELETE message is received it routes to the thread REMOVE was
 * called on. It looks at the Actor's Envelope Count (semaphore count?) on the
 * REMOVE'd Actor and if it is 0 then it removes it. It keeps sending the same
 * message if not.
 */
int
action_delete (lua_State *L);

/*
 * Load the Actor as per the data.
 * Expects a definition table on top of the Lua state.
 */
int
action_load (lua_State *L);

/*
 * Attempt to send the message to the actor. If the Actor is busy, this message
 * is sent again. Else the message is sent and then stops.  Expects a tone on
 * top, the author beneath the top, and the message beneath the author.
 */
int
action_send (lua_State *L);

/*
 * Terminate the calling thread.
 */
int
action_end (lua_State *L);

/*
 */
int
action_error (lua_State *L);

#endif
