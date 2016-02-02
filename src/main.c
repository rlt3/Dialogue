#include <stdlib.h>
#include <stdio.h>
#include "company.h"

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
    lua_State *L;

    if (argc == 1)
        usage(argv[0]);

    if (company_create(10, 20, 10) != 0)
        goto quit;

    L = luaL_newstate();

    if (!L)
        goto quit;

    luaL_openlibs(L);
    company_set(L);

    if (luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", argv[1],
                lua_tostring(L, -1));
        goto cleanup;
    }

    ret = 0;
cleanup:
    lua_close(L);
    company_close();
quit:
    return ret;
}
