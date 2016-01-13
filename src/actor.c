#include <pthread.h>
#include "actor.h"
#include "actor_meta.h"
#include "script.h"

typedef struct Actor {
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
} Actor;

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
 *
 * TODO:
 *  The root of a tree is always put into the interpreter state under the
 *  variable "rootN" where N is the number of Dialogue trees?
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

    /* Create Dialogue table structure without reinitializing the Director */
    lua_newtable(A);

    luaL_requiref(A, "Actor", luaopen_Dialogue_Actor, 1);
    lua_setfield(A, -2, "Actor")

    create_director_table(A);
    director_set(A, lua_gettop(A), director);
    lua_setfield(A, -2, "Director");

    lua_setglobal(A, "Dialogue");

    /* 
     * Allocate memory for the Actor inside its own Lua state. This means each
     * actor is a userdata *inside* its own Lua state, but lightuserdata 
     * everywhere else (Worker, Interpreter, etc).
     */
    actor = lua_newuserdata(A, sizeof(*actor));
    luaL_getmetatable(L, ACTOR_META);
    lua_setmetatable(L, -2);
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

    pthread_mutex_init(&actor->state_mutex, NULL);
    pthread_mutex_init(&actor->reference_mutex, NULL);
    pthread_rwlock_init(&actor->structure, NULL);
    
    /* 
     * If the first element of the definition table is a string, parse it for
     * 'Lead' or 'Star'. Increment script_position to skip the first element
     * when doing scripts later.
     */
    lua_rawgeti(W, definition_index, start);
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

    for (i = script_position; i <= len; i++) {
        lua_rawgeti(W, definition_index, i);
        script_create(W, actor);
        lua_pop(W, 1);
    }

    /* Call Dialogue.Director{ "load", actor } so scripts load asynchronously */
    lua_getglobal(A, "Dialogue");
    lua_getfield(A, -1, "Director");

    lua_newtable(A);

    lua_pushliteral(A, "load");
    lua_rawseti(A, -2, 1);

    lua_getglobal(A, "actor");
    lua_rawseti(A, -2, 2);

    lua_call(A, 1, 0);

    return actor;
}
