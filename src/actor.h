#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#include "dialogue.h"

#define ACTOR_LIB "Dialogue.Actor"

typedef struct Actor Actor;

/*
 * Create an Actor from a definition of Scripts, which are defined in Lua as
 * a table of tables.
 *
 * Actor{ {"draw", 400, 200}, {"weapon", "longsword"} }
 *
 * An optional string of "Lead" or "Star" can be placed in the first slot of a
 * table, which limits an Actor to a single thread or to the main thread 
 * respectively.
 *
 * This function accepts the Worker's state and expect the definition above to
 * be at the top of that state. The Actor is attached to the `parent' as a 
 * child.  The `parent' may be NULL to denote the created Actor is the root of
 * a Dialogue tree.
 */
Actor *
actor_create (lua_State *W, Actor *parent);

/*
 * Load an Actor's scripts. The `script_index' should be the index of a Script
 * from an Actor's definition table. With the following definition, the `draw'
 * Script is at index 1 and `weapon' at index 2.
 *     Actor{ {"draw", 400, 200}, {"weapon", "longsword"} }
 *
 * Alternatively, `script_index' can be 0 to load all Scripts of an Actor.
 *
 * Returns: LOAD_OK, LOAD_BAD_SCRIPT, LOAD_BAD_THREAD, LOAD_FAIL.
 *
 * In all cases except LOAD_OK, a message is printed to the console telling
 * which Script of which Actor caused the error.
 */
int
actor_load (Actor *actor, int script_index);

/*
 * Push a table onto the given Lua stack with the audience of an Actor by the
 * tone. This function is potentially blocking as there is a read/write lock on
 * an Actor's structure and the audience itself is just a collection of Actors.
 */
void
actor_audience (Actor *actor, lua_State *L, const char *tone);

/*
 * Remove an Actor from the Dialogue tree. Since the Dialogue tree is merely a
 * linked-list, it acquires a write lock on any structure referencing this 
 * Actor to remove it. That means this function will cause `actor_audience' to
 * block for a short period while the structure is changed.
 *
 * This does not free any memory of close an Actor's state, just removes it
 * from the Dialogue.
 */
void
actor_remove (Actor *actor);

/*
 * Close the Actor's Lua state and free the memory the Actor is using.
 */
void
actor_destroy (Actor *actor);

/*
 * Block for access to an Actor's structure. An Actor's structure is all of its
 * children, parent, and siblings.
 */
void
actor_request_structure (Actor *actor);

/*
 * Return access to an Actor's structure. Should follow 
 * 'actor_request_structure'.
 */
void
actor_return_structure (Actor *actor);

int 
luaopen_Dialogue_Actor (lua_State *L);

#endif
