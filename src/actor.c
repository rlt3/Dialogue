#include <pthread.h>
#include "actor.h"
#include "actor_meta.h"
#include "script.h"

struct Actor {
    lua_State *L;

    /* 
     * A read write lock for the structure of the Actor -- it's place in the
     * Dialogue tree -- it's parent, siblings, and children.
     */
    pthread_rwlock_t structure;

    /* For everything else including the state & scripts */
    pthread_mutex_t state_mutex;

    pthread_mutex_t reference_mutex;

    /* how many actions are referencing this actor? */
    int reference_count;

    /*
     * Restrict to single thread. If is_star is true, then restrict to main 
     * thread. If is_lead is true, then restrict to a Postman thread.
     */
    int is_lead;
    int is_star;

    /* Tree nav: go up, horizontally, and down the tree */
    struct Actor *parent;
    struct Actor *sibling;
    struct Actor *child;

    /* Go directly to the head of the tree */
    struct Actor *dialogue;

    struct Script *script_head;
    struct Script *script_tail;
};

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
 * be at the top of that state. The Director pointer is passed along because we
 * don't want to require (as in Lua's require) the Director module because that
 * would create a new set of workers.
 *
 * The Actor is attached to the `parent' as a child.  The `parent' may be NULL
 * to denote the created Actor is the root of a Dialogue tree.
 */
Actor *
actor_create (lua_State *W, Director *director, Actor *parent)
{
    Actor *actor;
    lua_State *A = luaL_newstate();
    int definition_index = lua_gettop(W);
    int len = luaL_len(W, definition_index);
    int i, script_position = 1;

    luaL_openlibs(A);

    /* Load the Actor module into the state so we can use metatable */
    luaL_requiref(A, "Actor", luaopen_Dialogue_Actor, 1);
    lua_pop(A, 1);

    /* 
     * Allocate memory for the Actor inside its own Lua state. This means each
     * actor is a userdata *inside* its own Lua state, but lightuserdata 
     * everywhere else (Worker, Interpreter, etc).
     */
    actor = lua_newuserdata(A, sizeof(*actor));
    luaL_getmetatable(A, ACTOR_META);
    lua_setmetatable(A, -2);
    lua_setglobal(A, "actor");

    actor->L = A;
    actor->parent = NULL;
    actor->sibling = NULL;
    actor->child = NULL;
    actor->dialogue = NULL;
    actor->script_head = NULL;
    actor->script_tail = NULL;
    actor->reference_count = 0;
    actor->is_lead = 0;
    actor->is_star = 0;

    /*
     * TODO:
    if (parent != NULL)
        actor_add_child(parent, actor);
     */

    pthread_mutex_init(&actor->state_mutex, NULL);
    pthread_mutex_init(&actor->reference_mutex, NULL);
    pthread_rwlock_init(&actor->structure, NULL);
    
    /* 
     * If the first element of the definition table is a string, parse it for
     * 'Lead' or 'Star'. Increment script_position to skip the first element
     * when doing scripts later.
     */
    lua_rawgeti(W, definition_index, script_position);
    if (lua_type(W, -1) == LUA_TSTRING) {
        script_position++;
        switch (lua_tostring(W, -1)[0]) {
        /* since first char of each string is different, switch through it */
        case 'S':
            actor->is_star = 1;
            break;
        case 'L':
            actor->is_lead = 1;
            break;
        default:
            break;
        }
    }
    lua_pop(W, 1);

    /*
     * TODO:
     *   Handle definitions which might want to `name' an Actor. An Actor's
     *   name lets us reference it by a string (its name) instead of pointer.
     *   Let's look at an example:
     *      {"deliver", "input", "yell", "up"}
     *   The 'input' Actor delivers the line `up' by yelling it to the entire
     *   Dialogue tree.
     *
     *   As of right now, I think named actors are handled in the main thread
     *   as "input", the string, can literally be interned as `input', the
     *   variable, in the main Lua state, which makes lookup easy.
     */

    for (i = script_position; i <= len; i++) {
        lua_rawgeti(W, definition_index, i);
        /*
         * TODO:
        script_create(W, actor);
         */
        lua_pop(W, 1);
    }

    printf("Creating Actor %p\n", actor);

    return actor;
}

/*
 * Close the Actor's Lua state and free the memory the Actor is using.
 */
void
actor_destroy (Actor *actor)
{
    printf("Destroying Actor %p\n", actor);
    pthread_mutex_destroy(&actor->reference_mutex);
    pthread_mutex_destroy(&actor->state_mutex);
    pthread_rwlock_destroy(&actor->structure);
    lua_close(actor->L);
}
