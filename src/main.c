#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "company.h"
#include "director.h"
#include "interpreter.h"

static lua_State *L;

void 
handle_sigint (int arg)
{
    director_close();
    lua_close(L);
    company_close();
    interpreter_destroy();
}

void
usage (const char *program)
{
    fprintf(stderr, "%s [script]\n", program);
    exit(1);
}

int
main (int argc, char **argv)
{
    int ret = 1;

    if (argc == 1)
        usage(argv[0]);

    /* 10 base actors, 20 max actors, 10 children each */
    if (company_create(10, 20, 10) != 0)
        goto quit;

    /* director with 2 workers */
    if (director_create(2) != 0)
        goto quit;

    if (interpreter_create() != 0)
        goto quit;

    L = luaL_newstate();

    if (!L)
        goto quit;

    luaL_openlibs(L);

    lua_newtable(L);

    lua_newtable(L);
    lua_setfield(L, -2, "Company");

    lua_newtable(L);
    lua_setfield(L, -2, "Director");

    lua_newtable(L);
    lua_setfield(L, -2, "Actor");

    lua_setglobal(L, "Dialogue");

    company_set(L);
    director_set(L);
    signal(SIGINT, handle_sigint);

    //if (luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0)) {
    //    fprintf(stderr, "File: %s could not load: %s\n", argv[1],
    //            lua_tostring(L, -1));
    //    goto cleanup;
    //}

    char *line;

    while (1) {
        if (interpreter_poll_input(&line) == 0)
            printf("INPUT: %s\n", line);
    }

    ret = 0;
cleanup:
    director_close();
    lua_close(L);
    company_close();
    interpreter_destroy();
quit:
    return ret;
}
