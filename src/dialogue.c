#include "dialogue.h"
#include "company.h"
#include "console.h"
#include "director.h"

#define DIALOGUE_META "Dialogue"

static int opts[] = {
    0, 4, 10, 20, 10, 
    1, 1
};

void
dialogue_option_set (enum DialogueOption option, int value)
{
    opts[option] = value;
}

void
dialogue_set_io_write (lua_State *L)
{
    if (opts[ACTOR_CONSOLE_WRITE])
        console_set_write(L);
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

    if (director_create(opts[WORKER_IS_MAIN], 
                        opts[WORKER_COUNT]) != 0)
        luaL_error(L, "Dialogue: Failed to create the Director and Workers!");

    company_set(L);
    director_set(L);

    lua_newtable(L);

    luaL_newmetatable(L, DIALOGUE_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, dialogue_metamethods, 0);

    lua_setmetatable(L, -2);

    return 1;
}
