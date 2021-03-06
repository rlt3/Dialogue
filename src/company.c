#include <stdlib.h>
#include <assert.h>
#include "company.h"
#include "tree.h"
#include "director.h"
#include "actor.h"
#include "utils.h"

#define COMPANY_META "Dialogue.Company"
#define ACTOR_META "Dialogue.Company.Actor"

/*
 * Create the Company tree with the number of actors.
 */
int
company_create (int num_actors)
{
    return tree_init(num_actors, actor_assign_id, actor_destroy);
}

/*
 * Add an Actor to the Company. Expects an Actor's definition table on top of
 * L. If thread_id > -1 then the created Actor will only be ran on the thread
 * with that id (thread_id == 0 is the main thread).  Returns the id of the new
 * Actor if successful otherwise it will call lua_error on L. 
 */
int 
company_add (lua_State *L, int parent, int thread_id)
{
    int id = tree_add_reference(actor_create(L), parent, thread_id);

    switch (id) {
    case TREE_ERROR:
        luaL_error(L, "Failed to create actor: max actors reached!");
        break;

    case NODE_ERROR:
        luaL_error(L, "Failed to create actor: invalid parent id `%d`!", 
                parent);
        break;

    case NODE_INVALID:
        luaL_error(L, "Failed to create actor: resize failed for parent `%d`!", 
                parent);
        break;

    default:
        break;
    }

    return id;
}

/*
 * Remove an actor from the Company's Tree but still leave it accessible in
 * memory (to be reloaded or otherwise tested).
 */
void
company_bench (lua_State *L, int id)
{
    if (tree_unlink_reference(id, 0) != 0)
        luaL_error(L, "Cannot bench invalid reference `%d`!", id);
}

/*
 * Join an actor which was benched back into the Company's tree. If the parent
 * is >NODE_INVALID then the benched Actor is joined as a child of that parent.
 */
void
company_join (lua_State *L, const int id, const int parent)
{
    switch (tree_link_reference(id, parent)) {
    case TREE_ERROR:
        luaL_error(L, "Cannot join `%d`: bad parent!", id);
        break;

    case NODE_ERROR:
        luaL_error(L, "Cannot join `%d`: not benched!", id);
        break;

    case NODE_INVALID:
        luaL_error(L, "Cannot join `%d`: id invalid!", id);
        break;

    default:
        break;
    }
}

/*
 * Remove an Actor from the Company's Tree and mark it as garbage. Will error
 * through L if the id is invalid.
 */
void
company_delete (lua_State *L, int id)
{
    if (tree_unlink_reference(id, 1) != 0)
        luaL_error(L, "Cannot delete invalid reference `%d`!", id);
}

/*
 * Get the actor associated with the id. Will error out on L if the id isn't a
 * valid reference. Requires called `company_deref` is the return was *not*
 * NULL.
 */
Actor *
company_ref (lua_State *L, int id)
{
    Actor *actor = tree_ref(id);
    if (!actor)
        luaL_error(L, "Actor id `%d` is an invalid reference!", id);
    return actor;
}

/*
 * Return the actor associated with the id. Calling this function for an id
 * that returned NULL for `company_ref` produces undefined behavior.
 */
int
company_deref (int id)
{
    return tree_deref(id);
}

/*
 * Callback for calling 'unload' on each actor in the company.
 */
void
company_unload_actor_callback (void *data, int id)
{
    lua_State *L = data;

    /* actor:async("unload") */
    company_push_actor(L, id);
    lua_getfield(L, -1, "async");
    lua_pushvalue(L, -2); /* self */
    lua_pushliteral(L, "unload");
    /* protected call just to catch errors. */
    lua_pcall(L, 2, 0, 0);
    lua_pop(L, 1); /* actor */
}

/*
 * Call the 'destroy' method for the Scripts of the Company's Actors starting
 * from the root of the tree. This function is meant to be called before 
 * closing the Company and the Workers.
 */
void
company_cleanup (lua_State *L)
{
    tree_map_subtree(tree_root(), company_unload_actor_callback, L, 
            TREE_READ, TREE_RECURSE);
}

/*
 * Destroy the Company and all the Actors.
 */
void
company_close ()
{
    tree_cleanup();
}

/*
 * Push an Actor reference object onto the Lua stack.
 */
void
company_push_actor (lua_State *L, int actor_id)
{
    lua_newtable(L);

    lua_pushinteger(L, actor_id);
    lua_rawseti(L, -2, 1);

    luaL_getmetatable(L, ACTOR_META);
    lua_setmetatable(L, -2);
}

/*
 * Push all the ids to the table on top of the stack.
 */
void
company_audience_callback (void* data, const int id)
{
    lua_State *L = data;
    lua_pushinteger(L, id);
    lua_rawseti(L, -2, luaL_len(L, -2) + 1);
}

/*
 * Callback data for the tree_map_subtree function. It accepts void* so we 
 * just passed the address of the stack pointer for the data.
 */
struct company_callback_data {
    lua_State *L;
    int id;
};

/*
 * Push all the ids to the table on top of the stack except the parent's.
 */
void
company_children_callback (void *data, const int id)
{
    struct company_callback_data *c = data;
    lua_State *L = c->L;
    const int parent = c->id;

    if (parent == id)
        return;

    lua_pushinteger(L, id);
    lua_rawseti(L, -2, luaL_len(L, -2) + 1);
}

/*
 * Pushes a table of actor ids which correspond to the audience of the actor by
 * the tone.
 */
void
company_push_audience (lua_State *L, int id, const char *tone)
{
    /*
     * Yell is recursive from the Root node.
     * Command is recursive from `id` node.
     * Say is non-recursive from the parent of `id` node.
     * Neither think nor whisper need a tree operation.
     */

    lua_newtable(L);

    switch (tone[0]) {
    case 'y': case 'Y':
        tree_map_subtree(tree_root(), company_audience_callback, L,
                TREE_READ, TREE_RECURSE);
        break;

    case 'c': case 'C':
        tree_map_subtree(id, company_audience_callback, L,
                TREE_READ, TREE_RECURSE);
        break;

    case 's': case 'S':
        tree_map_subtree(tree_node_parent(id), company_audience_callback, L,
                TREE_READ, TREE_NON_RECURSE);
        break;

    default:
        //lua_pop(L, 1);
        return;
    }
}

/*
 * An actor can be represented in many ways. All of them boil down to an id.
 * This function returns the id of an actor at index. Will call lua_error on
 * L if the type is unexpected.
 */
int
company_actor_id (lua_State *L, int index)
{
    const char *error_type = "Unkown Type!";
    int id = NODE_INVALID;
    int type = lua_type(L, index);

    switch(type) {
    case LUA_TTABLE:
        lua_rawgeti(L, index, 1);
        id = lua_tointeger(L, -1);
        lua_pop(L, 1);
        break;

    case LUA_TNUMBER:
        id = lua_tointeger(L, index);
        break;
        
    case LUA_TSTRING:
        error_type = "string";
        goto error;

    case LUA_TFUNCTION:
        error_type = "function";
        goto error;

    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
        error_type = "userdata";
        goto error;

    case LUA_TBOOLEAN:
        error_type = "boolean";
        goto error;

    case LUA_TTHREAD:
        error_type = "thread";
        goto error;

    case LUA_TNIL:
        error_type = "nil";
error:
    default:
        luaL_error(L, "Cannot coerce Actor id from type `%s`!", error_type);
        break;
    }

    return id;
}

/*
 * Return the parent of the actor with id.
 * Will error through L if the actor at id isn't used.
 */
int
company_actor_parent (lua_State *L, const int id)
{
    int ret = tree_node_parent(id);
    switch (ret) {
    case TREE_ERROR:
        luaL_error(L, 
            "Failed getting Actor `%s`'s parent: read lock failed!", id);
        break;

    case NODE_ERROR:
        luaL_error(L, "Cannot get parent of invalid Actor `%d`!", id);
        break;
    }
    return ret;
}

/*
 * Cleanups a garbage (removed) Actor. Errors through L if the Actor isn't 
 * garbage.
 */
void
company_actor_cleanup (lua_State *L, const int id)
{
    switch (tree_node_cleanup(id)) {
    case TREE_ERROR:
        luaL_error(L, 
            "Cleanup failed! Locks for garbage Actor `%d` failed!", id);
        break;

    case NODE_ERROR:
        luaL_error(L, "Cleanup failed! Actor `%d` is not garbage!", id);
        break;
    }
}

/*
 * Check if the current thread is invalid for the actor at id. Returns 1 if so,
 * 0 otherwise.
 */
static inline int
company_actor_invalid_thread (lua_State *L, const int id)
{
    int ret = 0;
    const int thread_id = tree_node_thread(id);

    if (thread_id > NODE_INVALID) {
        /* __worker_id global is set in each worker state */
        lua_getglobal(L, "__worker_id");
        ret = (lua_isnil(L, -1) || lua_tointeger(L, -1) != thread_id);
        lua_pop(L, 1);
    }

    return ret;
}

typedef int (*ActorFunc) (Actor*, lua_State*);

/*
 * Call the Actor function with the instance from the id. 
 *
 * Errors out if that actor has a specific thread requirement and we aren't in
 * that specific thread. 
 * Errors out if `func` fails.
 * Errors out if the actor id is invalid.
 *
 * TODO: pass string to function so we can have more descriptive errors
 * from this function to the user about which actor function caused the
 * error and why
 */
void
company_call_actor_func (lua_State *L, int id, ActorFunc func)
{
    Actor *actor = NULL;

    if (company_actor_invalid_thread(L, id))
        luaL_error(L, "Actor `%d` has a worker requirement not met!", id);

    actor = company_ref(L, id);

    /* if actor_send doesn't return 0, it puts an error string on top of A */
    if (func(actor, L) != 0) {
        actor_pop_error(actor, L);
        company_deref(id);
        lua_error(L);
    }

    company_deref(id);
}

/*
 * Actor( definition_table [, parent] [, thread] )
 *
 * Creates the actor from the given definition table (see actor.h for more
 * info). An optional Actor object that should be the parent of the created 
 * Actor can be passed.
 */
int
lua_actor_new (lua_State *L)
{
    const int definition_arg = 2;
    const int parent_arg = 3;
    const int thread_arg = 4;
    int args = lua_gettop(L);
    int parent = NODE_INVALID;
    int thread = NODE_INVALID;
    
    luaL_checktype(L, definition_arg, LUA_TTABLE);

    /* get the thread id, then pop it and any extra args */
    if (args >= thread_arg) {
        thread = luaL_checkinteger(L, thread_arg);
        lua_pop(L, args - thread_arg + 1);
        args = parent_arg;
    }

    /* get the parent id, then pop it and extra args, leaving table on top */
    if (args >= parent_arg) {
        parent = company_actor_id(L, parent_arg);
        lua_pop(L, args - parent_arg + 1);
    }

    assert(lua_gettop(L) == definition_arg);

    company_push_actor(L, company_add(L, parent, thread));

    if (dialogue_actor_manual_load())
        goto exit;

    /* actor:async('load') */
    lua_getfield(L, -1, "async");
    lua_pushvalue(L, -2);
    lua_pushliteral(L, "load");
    lua_call(L, 2, 0);

exit:
    return 1;
}

/*
 * The top-level dispatch function of the Company. This function can be called
 * two ways right now: one can create an Actor and push Lua object to reference
 * that Actor, or it can just push a Lua object to reference an id.
 *
 * When creating an actor, it will error through Lua with a descriptive error
 * message. If not creating an actor (and just pushing a reference), it does no
 * checks on the id given.
 *
 * Actor{ {"draw", 200, 400} } => actor
 * Actor(20) => actor
 */
int
lua_company_call (lua_State *L)
{
    const int bottom = 2;
    const int args = lua_gettop(L);

    if (args >= bottom && lua_type(L, bottom) == LUA_TTABLE) {
        lua_actor_new(L);
        goto exit;
    }

    if (args == bottom) {
        company_push_actor(L, company_actor_id(L, bottom));
        goto exit;
    }

    luaL_error(L, "Invalid # of arguments to `Actor`. Expected >= %d got %d",
            bottom, args);

exit:
    return 1;
}

/*
 * Load the actor. If a specific script needs to be reloaded, pass the id of 
 * that script. If you would like to reload all the scripts forcefully, pass
 * the string "all".
 *
 * actor:load()
 * actor:load(2)
 * actor:load("all")
 */
int
lua_actor_load (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    company_call_actor_func(L, id, actor_load);
    return 0;
}

/*
 * Unload the scripts of an actor. If the integer `1` is passed, this function
 * will delete the actor as well as unload it.
 *
 * actor:unload([1])
 */
int
lua_actor_unload (lua_State *L)
{
    const int actor_arg = 1;
    const int delete_opt_arg = 2;
    const int is_delete = luaL_optinteger(L, delete_opt_arg, 0);
    const int id = company_actor_id(L, actor_arg);
    company_call_actor_func(L, id, actor_unload);
    if (is_delete)
        company_delete(L, id);
    return 0;
}

/*
 * Create a child actor with this object as the parent.
 * actor:child({ definition_table } [, thread])
 */
int
lua_actor_child (lua_State *L)
{
    const int self_arg = 1;
    const int table_arg = 2;
    const int thread_opt_arg = 3;
    const int thread = luaL_optinteger(L, thread_opt_arg, NODE_INVALID);
    int args = 3;

    lua_pushcfunction(L, lua_actor_new);
    lua_pushnil(L); /* a leroy special */
    lua_pushvalue(L, table_arg);
    lua_pushvalue(L, self_arg); /* parent */

    if (thread != NODE_INVALID) {
        lua_pushinteger(L, thread);
        args++;
    }

    lua_call(L, args, 1);
    return 1;
}

/*
 * Return the children of the actor as an array of actor ids.
 * actor:children()
 */
int
lua_actor_children (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    struct company_callback_data data = { L, id };
    lua_newtable(L);
    tree_map_subtree(id, company_children_callback, &data, 
            TREE_READ, TREE_NON_RECURSE);
    return 1;
}

/*
 * Bench an actor from the Tree. This removes it as a child from its parent. It
 * won't show up in the audience of any other Actor, but it still exists and can
 * be reloaded, changed, etc. A benched Actor can be rejoined to the tree with
 * `actor:join()`
 *
 * actor:bench()
 */
int
lua_actor_bench (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    company_bench(L, id);
    return 0;
}

/*
 * Join a benched Actor back to the tree. If no parent is specified, the Actor
 * is joined as a child of its old parent. Otherwise, it is joined as a child
 * of the provided parent.
 *
 * actor:join([new parent])
 */
int
lua_actor_join (lua_State *L)
{
    const int actor_arg = 1;
    const int parent_opt_arg = 2;
    const int id = company_actor_id(L, actor_arg);
    const int parent = luaL_optinteger(L, parent_opt_arg, NODE_INVALID);
    company_join(L, id, parent);
    return 0;
}

/*
 * This is a convenience method for removing an actor that checks for
 * thread requirements of the actor. This calls actor:unload with the delete
 * option, asynchronously if it doesn't meet the thread reqs.
 *
 * Unload and remove an actor from the company, marking it as garbage. This is
 * permanent and cannot be undone. Please see `actor:bench()` to temporarily 
 * remove an Actor from the tree.
 *
 * actor:remove()
 */
int
lua_actor_remove (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);

    if (company_actor_invalid_thread(L, id)) {
        lua_getfield(L, actor_arg, "async");
        lua_pushvalue(L, actor_arg); /* self */
        lua_pushliteral(L, "unload");
        lua_pushinteger(L, 1);
        lua_call(L, 3, 0);
        goto exit;
    }

    lua_pushinteger(L, 1);
    lua_actor_unload(L);
exit:
    return 0;
}

/*
 * Cleanup an Actor after it has been deleted but before it has been garbage 
 * collected. This effectively "garbage collects" the Actor.
 */
int
lua_actor_cleanup (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    company_actor_cleanup(L, id);
    return 0;
}

/*
 * actor:lock()
 *
 * Locks the specific actor. This will cause all threads that *need* (and can't
 * skip) the reference to block and wait. This will also cause memory leaks if
 * the actor isn't unlocked before the system quits.
 * Errors on L if the id was bad somehow.
 */
int
lua_actor_lock (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    company_ref(L, id);
    return 0;
}

/*
 * actor:unlock()
 *
 * Unlocks a previously locked actor. Unlocking a not-locked actor is undefined.
 */
int
lua_actor_unlock (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    company_deref(id);
    return 0;
}

/*
 * Return the actor's id.
 * actor:id()
 */
int
lua_actor_id (lua_State *L)
{
    const int actor_arg = 1;
    lua_pushinteger(L, company_actor_id(L, actor_arg));
    return 1;
}

/*
 * Return the id of the actor's parent.
 * actor:parent()
 */
int
lua_actor_parent (lua_State *L)
{
    const int actor_arg = 1;
    company_push_actor(L, 
            company_actor_parent(L, 
                company_actor_id(L, actor_arg)));
    return 1;
}

/*
 * Send a messsage to the actor in the form of:
 * { 'message' [, arg1 [, ... [, argn]]], author}
 *
 * a0 = Actor{ {"graphics", 200, 400} }
 * a0:send{"draw", 20, 20, a0}
 */
int
lua_actor_send (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    company_call_actor_func(L, id, actor_send);
    return 1;
}

/*
 * Probe a specific Script's object for the given field.
 *
 * actor:probe(script_id, field_name)
 *
 * a0 = Actor{ {"graphics", 200, 400} }
 * a0:probe(1, "width") => 200
 * a0:probe(1, "height") => 400
 */
int
lua_actor_probe (lua_State *L)
{
    const int actor_arg = 1;
    const int id = company_actor_id(L, actor_arg);
    company_call_actor_func(L, id, actor_probe);
    return 1;
}

/*
 * Create an Action for the Director for the actor. "Task" it with doing the
 * method (with the given arguments).
 *
 * actor:async("send", "draw", 50, 50) => {actor, "send", "draw", 50, 50}
 * actor:async("load") => {actor, "load"}
 */
int
lua_actor_async (lua_State *L)
{
    const int self_arg = 1;
    const int args = lua_gettop(L);
    const int id = company_actor_id(L, self_arg);
    const int thread_id = tree_node_thread(id);
    int i, call_args = 1;

    if (thread_id == NODE_ERROR)
        luaL_error(L, 
                "Starting async method `%s` failed: invalid Actor id `%d`!", 
                lua_tostring(L, self_arg + 1), id);

    lua_pushcfunction(L, director_take_action);
    lua_newtable(L);

    for (i = self_arg; i <= args; i++) {
        lua_pushvalue(L, i);
        lua_rawseti(L, -2, i);
    }

    if (thread_id > NODE_INVALID) {
        lua_pushinteger(L, thread_id);
        call_args++;
    }

    lua_call(L, call_args, 0);

    return 0;
}

/*
 * Push the audience of an Actor by the tone it's using. The audience is just 
 * an array (table) of actor ids.
 * 
 * actor:audience("say")
 * actor:audience("yell")
 */
int
lua_actor_audience (lua_State *L)
{
    const int self_arg = 1;
    const int tone_arg = 2;
    const int id = company_actor_id(L, self_arg);
    const char *tone = luaL_checkstring(L, tone_arg);
    company_push_audience(L, id, tone);
    return 1;
}

int
company_actor_tone (lua_State *L, const char *tone)
{
    const int actor_arg = 1;
    const int message_arg = 2;
    const int audience_index = 3;
    const int id = company_actor_id(L, actor_arg);
    int i;

    /* append the actor's id to the message (set the author) */
    lua_pushinteger(L, id);
    lua_rawseti(L, message_arg, luaL_len(L, message_arg) + 1);

    company_push_audience(L, id, tone);

    /* foreach (actor : audience) { actor:async("send", {msg}) } */
    for (i = 1; i <= luaL_len(L, audience_index); i++) {
        lua_pushcfunction(L, lua_actor_async);
        lua_rawgeti(L, audience_index, i);
        lua_pushliteral(L, "send");
        lua_pushvalue(L, message_arg);
        lua_call(L, 3, 0);
    }

    return 0;
}

/*
 * Yell a message to the entire Company.
 * actor:yell{"attack", "B", "4"}
 */
int
lua_actor_yell (lua_State *L)
{
    return company_actor_tone(L, "yell");
}

/*
 * Send a message to the Actor's children
 * actor:command{"does_collide", {4, 6}, {5, 3}}
 */
int
lua_actor_command (lua_State *L)
{
    return company_actor_tone(L, "command");
}

/*
 * Send a message to itself and its siblings (the children of its parent).
 * actor:say{"moving", 2, 4}
 */
int
lua_actor_say (lua_State *L)
{
    return company_actor_tone(L, "say");
}

/*
 * Send a message to one other Actor.
 * actor:whisper(other, {"attack", dmg})
 */
int
lua_actor_whisper (lua_State *L)
{
    const int actor_arg = 1;
    const int recipient_arg = 2;
    const int message_arg = 3;
    const int author_id = company_actor_id(L, actor_arg);
    company_actor_id(L, recipient_arg);

    /* append the actor's id to the message (set the author) */
    lua_pushinteger(L, author_id);
    lua_rawseti(L, message_arg, luaL_len(L, message_arg) + 1);

    lua_pushcfunction(L, lua_actor_async);
    lua_pushvalue(L, recipient_arg);
    lua_pushliteral(L, "send");
    lua_pushvalue(L, message_arg);
    lua_call(L, 3, 0);
    return 0;
}

/*
 * Send a message to itself.
 * actor:think{"update", dt}
 */
int
lua_actor_think (lua_State *L)
{
    const int actor_arg = 1;
    const int message_arg = 2;
    const int author_id = company_actor_id(L, actor_arg);

    /* append the actor's id to the message (set the author) */
    lua_pushinteger(L, author_id);
    lua_rawseti(L, message_arg, luaL_len(L, message_arg) + 1);

    lua_pushcfunction(L, lua_actor_async);
    lua_pushvalue(L, actor_arg);
    lua_pushliteral(L, "send");
    lua_pushvalue(L, message_arg);
    lua_call(L, 3, 0);
    return 0;
}

static const luaL_Reg actor_metamethods[] = {
    {"load",     lua_actor_load},
    {"unload",   lua_actor_unload},
    {"child",    lua_actor_child},
    {"children", lua_actor_children},
    {"remove",   lua_actor_remove},
    {"cleanup",  lua_actor_cleanup},
    {"lock",     lua_actor_lock},
    {"unlock",   lua_actor_unlock},
    {"id",       lua_actor_id},
    {"parent",   lua_actor_parent},
    {"bench",    lua_actor_bench},
    {"join",     lua_actor_join},
    {"send",     lua_actor_send},
    {"probe",    lua_actor_probe},
    {"async",    lua_actor_async},
    {"audience", lua_actor_audience},
    {"yell",     lua_actor_yell},
    {"command",  lua_actor_command},
    {"say",      lua_actor_say},
    {"whisper",  lua_actor_whisper},
    {"think",    lua_actor_think},
    { NULL, NULL }
};

static const luaL_Reg company_metamethods[] = {
    {"__call",   lua_company_call},
    { NULL, NULL }
};

int
luaopen_Dialogue_Company (lua_State *L)
{
    luaL_newmetatable(L, ACTOR_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, actor_metamethods, 0);

    lua_newtable(L);

    luaL_newmetatable(L, COMPANY_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, company_metamethods, 0);

    lua_setmetatable(L, -2);

    return 1;
}

/*
 * Set the Company's table inside the given Lua state.
 */
void
company_set (lua_State *L)
{
    luaL_requiref(L, "Actor", luaopen_Dialogue_Company, 1);
    lua_pop(L, 1);
}
