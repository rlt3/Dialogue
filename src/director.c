#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "director.h"
#include "action.h"
#include "worker.h"
#include "mailbox.h"

struct Director {
    Worker **workers;
    int worker_count;
    Mailbox *main_mailbox;
    int rand_seed;
    struct timeval start;
    struct timeval now;
};

enum Actions {
    ACTION_SEND,
    ACTION_LOAD,
    ACTION_TONE,
    ACTION_BENCH,
    ACTION_JOIN,
    ACTION_DELETE
};

static Director *global_director = NULL;

static const char *director_field = "__ptr";

int
director_create (const int workers)
{
    const int director_table = 1;
    int ret = 1;
    int i, j;

    global_director = malloc(sizeof(*director));

    if (!global_director)
        goto exit;

    director->workers = malloc(sizeof(Worker*) * workers);

    if (!director->workers) {
        free(director);
        goto exit;
    }

    director->mailbox = mailbox_create();

    if (!director->mailbox) {
        free(director->workers);
        free(director);
        goto exit;
    }

    for (i = 0; i < director->worker_count; i++) { 
        director->workers[i] = worker_start(L, director);
        
        if (!director->workers[i]) {
            for (j = 0; j < i; j++)
                worker_cleanup(director->workers[j])
            free(director->mailbox);
            free(director->workers);
            free(director);
            goto exit;
        }
    }

    director->rand_seed = time(NULL);
    srand(director->rand_seed);

    gettimeofday(&director->start, NULL);
    director->now = director->start;

    ret = 0;
exit:
    return ret;
}

/*
 * Set the Director to a table at the index.
 */
void
director_set (lua_State *L, int index, Director *director)
{
    lua_pushlightuserdata(L, director);
    lua_setfield(L, index, director_field);
}

/*
 * Returns the Director in the given Lua state. If the Director doesn't exist, 
 * one is created. This function looks for 'Dialogue.Director.worker_count' 
 * (defaults to 4) to see how many Workers are spawned to help the Director.
 * The Director creates its own Mailbox and acts as a Worker specifically for
 * the Main thread. The Director also starts Dialogue's clock.
 */
Director *
director_or_init (lua_State *L)
{
    Director *director;
    const int director_table = 1;
    const int default_workers = 4;
    int i;

    /* try and return the director director_field if it exists already */
    lua_getfield(L, director_table, director_field);
    if (!lua_isnil(L, -1)) {
        director = lua_touserdata(L, -1);
        lua_pop(L, 1);
        goto exit;
    }
    lua_pop(L, 1);

    director = malloc(sizeof(*director));

    if (director == NULL)
        luaL_error(L, "Not enough memory for Director!");

    lua_getfield(L, director_table, "worker_count");
    director->worker_count = luaL_optinteger(L, -1, default_workers);
    lua_pop(L, 1);

    director->workers = malloc(sizeof(Worker*) * director->worker_count);
    if (director->workers == NULL) {
        free(director);
        luaL_error(L, "Not enough memory for the Director workers!");
    }

    for (i = 0; i < director->worker_count; i++) 
        director->workers[i] = worker_start(L, director);

    director->rand_seed = time(NULL);
    director->mailbox = mailbox_create();
    srand(director->rand_seed);
    gettimeofday(&director->start, NULL);

    director_set(L, director_table, director);

exit:
    return director;
}

/*
 * Receive an action in this form:
 *     Director{ action, actor [, data1 [, ... [, dataN]]] }
 *
 * Send the action to an open worker.
 */
int
lua_director_action (lua_State *L)
{
    Director *director;
    const int action_arg = 2;
    int count, start, i;

    luaL_checktype(L, action_arg, LUA_TTABLE);

    director = director_or_init(L);
    count = director->worker_count;
    start = rand() % count;

    lua_rawgeti(L, action_arg, 2);
    lua_pop(L, 1);

    for (i = start; i < count; i = (i + 1) % count)
        if (worker_take_action(L, director->workers[i]))
            break;

    return 0;
}

/*
 * Stop any threads and free any memory associated with them and the director.
 */
int
lua_director_quit (lua_State *L)
{
    int i;
    Director *director = director_or_init(L);

    /* stop everything first */
    for (i = 0; i < director->worker_count; i++)
        worker_stop(director->workers[i]);

    /* then cleanup, it avoids a lot of problems */
    for (i = 0; i < director->worker_count; i++)
        worker_cleanup(director->workers[i]);

    gettimeofday(&director->stop, NULL);
    printf("%f\n", 
        (double)(director->stop.tv_usec - director->start.tv_usec) / 1000000 
        + (double)(director->stop.tv_sec - director->start.tv_sec));

    mailbox_destroy(director->mailbox);
    free(director->workers);
    free(director);

    return 0;
}

int
lua_director_tostring (lua_State *L)
{
    Director *director = director_or_init(L);
    lua_pushfstring(L, "Dialouge.Director %p", director);
    return 1;
}

static const luaL_Reg director_actions[] = {
    {"new",     lua_action_create},
    {"bench",   lua_action_bench},
    {"join",    lua_action_join},
    {"receive", lua_action_receive},
    {"send",    lua_action_send},
    {"load",    lua_action_load},
    {"error",   lua_action_error},
    { NULL, NULL }
};

static const luaL_Reg director_metamethods[] = {
    {"__call",     lua_director_action},
    {"__tostring", lua_director_tostring},
    { NULL, NULL }
};

/*
 * Create the Director table with all the Actions in the given Lua state. This
 * function is used to push the Director to the Workers' states without the
 * gc metamethod so that each Worker is using the same exact Actions without
 * worrying about garbage collection.
 */
void
create_director_table (lua_State *L)
{
    lua_newtable(L);
    luaL_setfuncs(L, director_actions, 0);

    luaL_newmetatable(L, "Director");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, director_metamethods, 0);

    lua_setmetatable(L, -2);
}

/*
 * Create the Director table using above, but add the garbage collection
 * function to the metatable.
 */
int
luaopen_Dialogue_Director (lua_State *L)
{
    create_director_table(L);
    lua_getmetatable(L, -1);
    lua_pushcfunction(L, lua_director_quit);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}
