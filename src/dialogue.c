#include "dialogue.h"
#include "company.h"
#include "console.h"
#include "director.h"

#define DIALOGUE_META "Dialogue"

static int opts[] = {
    0, 4, 64, 10, 
    0, 0, 0
};

void
dialogue_option_set (enum DialogueOption option, int value)
{
    opts[option] = value;
}

int
dialogue_forced_synchronous ()
{
    return !opts[ACTOR_FORCE_SYNC];
}

int
dialogue_actor_manual_load ()
{
    return opts[ACTOR_MANUAL_LOAD];
}

void
dialogue_set_io_write (lua_State *L)
{
#ifndef DIALOGUE_MODULE
    if (opts[ACTOR_CONSOLE_WRITE])
        console_set_write(L);
#endif
}

int 
dialogue_cleanup (lua_State *L)
{
    company_cleanup(L);
    director_close();
    company_close();
    return 0;
}

static const luaL_Reg dialogue_metamethods[] = {
    {"__gc",   dialogue_cleanup},
    { NULL, NULL }
};

/*
 * TODO:
 *  Args can be passed to require like this, where ... can be a table.
 *      `Dialogue = require 'Dialogue' (...)`
 *
 *  Change the above option system to look for that table which uses a name 
 *  that is statically defined above.
 *
 *       `{ WORKER_IS_MAIN , "main_thread_is_worker", 0 }`
 *       `{   WORKER_COUNT , "worker_count", 4 }`
 *
 *  Then the array is updated as the iterator makes its way through each row.
 *  It looks the table for the key (mid column), if it exists, updates index 
 *  (first column). third column is default.
 *
 *  This lets the program's options be set through the command-line switches,
 *  the script, and through require if compiled the shared object.
 */
int
luaopen_Dialogue (lua_State *L)
{
    printf("top: %d\n", lua_gettop(L));

    return 0;

    //if (company_create(opts[ACTOR_COUNT]) != 0)
    //    luaL_error(L, "Dialogue: Failed to create the Company of Actors!");

    //if (director_create(opts[WORKER_IS_MAIN], 
    //                    opts[WORKER_COUNT]) != 0)
    //    luaL_error(L, "Dialogue: Failed to create the Director and Workers!");

    //company_set(L);
    //director_set(L);

    //lua_newtable(L);

    //luaL_newmetatable(L, DIALOGUE_META);
    //lua_pushvalue(L, -1);
    //lua_setfield(L, -1, "__index");
    //luaL_setfuncs(L, dialogue_metamethods, 0);

    //lua_setmetatable(L, -2);

    //return 0;
}
