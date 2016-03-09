#include "dialogue.h"
#include "company.h"
#include "director.h"

#define DIALOGUE_META "Dialogue"

static int opts[] = {
    0, 4, 10, 20, 10, 
    1
};

void
dialogue_option_set (enum DialogueOption option, int value)
{
    opts[option] = value;
}

int 
dialogue_cleanup (lua_State *L)
{
    director_close();
    company_close();
    return 0;
}

static const luaL_Reg dialogue_metamethods[] = {
    {"__gc",   dialogue_cleanup},
    { NULL, NULL }
};

int
luaopen_Dialogue (lua_State *L)
{
    if (company_create(opts[ACTOR_BASE], 
                       opts[ACTOR_MAX],
                       opts[ACTOR_CHILD_MAX]) != 0)
        luaL_error(L, "Dialogue: Failed to create the Company of Actors!");

    if (director_create(opts[WORKER_COUNT]) != 0)
        luaL_error(L, "Dialogue: Failed to create the Director and Workers!");

    company_set(L);
    director_set(L);

    luaL_newmetatable(L, DIALOGUE_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, dialogue_metamethods, 0);

    lua_newtable(L);
    lua_setmetatable(L, -2);

    /*
     * TODO:
     *    We can use this `require` function to load the main parts of the 
     *    program. If the Lua state has `DIALOGUE_MAIN_THREAD` as true then the
     *    Director will use the main thread as a Worker thread and will count
     *    toward the worker count. If `DIALOGUE_MAIN_THREAD` is nil then just
     *    `worker_count` workers (threads) are spawned.
     *
     *    The options will have to be done through some global struct. I'm 
     *    imagining some static struct with default values. It gets updated
     *    from the console through command-line switches.
     *
     *    dialogue_option_set(DIRECTOR_WORKERS, worker_count);
     *    dialogue_option_set(DIRECTOR_MAX_ACTORS, max_actors);
     *
     *    The Director/Company will be tied to this Lua state. A metatable will
     *    be created for the __gc metamethod so that closing this Lua state i
     *    the only requirement for cleaning up Dialogue. This extends to the
     *    main & module-only builds of the program.
     */

    return 1;
}
