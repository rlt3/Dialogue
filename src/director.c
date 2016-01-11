#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "director.h"
#include "worker.h"
#include "mailbox.h"

struct Director {
    Worker **workers;
    Mailbox *mailbox;
    int worker_count;
    int rand_seed;
    struct timeval stop, start;
};

static const char *pointer = "__ptr";

/*
 * Returns the Director of the Dialogue in the given Lua state. Initializes it
 * if not done already.
 */
Director *
director_or_init (lua_State *L)
{
    Director *director;
    const int dialogue_table = 1;
    const int default_workers = 4;
    int i;

    /* try and return the director pointer if it exists already */
    lua_getfield(L, dialogue_table, pointer);
    if (!lua_isnil(L, -1)) {
        director = lua_touserdata(L, -1);
        lua_pop(L, 1);
        goto exit;
    }
    lua_pop(L, 1);

    director = malloc(sizeof(*director));

    if (director == NULL)
        luaL_error(L, "Not enough memory for Director!");

    lua_getfield(L, dialogue_table, "worker_count");
    director->worker_count = luaL_optinteger(L, -1, default_workers);
    director->rand_seed = time(NULL);
    lua_pop(L, 1);

    director->workers = malloc(sizeof(Worker*) * director->worker_count);
    if (director->workers == NULL) {
        free(director);
        luaL_error(L, "Not enough memory for the Director workers!");
    }

    for (i = 0; i < director->worker_count; i++) 
        director->workers[i] = worker_start(L);

    lua_pushlightuserdata(L, director);
    lua_setfield(L, dialogue_table, pointer);

    srand(director->rand_seed);
    gettimeofday(&director->start, NULL);

exit:
    return director;
}

/*
 * Receive an action in this form:
 *     Dialogue{ action, actor [, data1 [, ... [, dataN]]] }
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

int
lua_director_quit (lua_State *L)
{
    int i;
    Director *director = director_or_init(L);

    printf("gcing\n");

    for (i = 0; i < director->worker_count; i++)
        worker_stop(L, director->workers[i]);

    gettimeofday(&director->stop, NULL);
    printf("%f\n", 
        (double)(director->stop.tv_usec - director->start.tv_usec) / 1000000 
        + (double)(director->stop.tv_sec - director->start.tv_sec));

    free(director->workers);
    free(director);

    return 0;
}

int
lua_director_tostring (lua_State *L)
{
    lua_pushstring(L, "Dialogue!");
    return 1;
}
