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
    lua_State *L;

    if (argc == 1)
        usage(argv[0]);

    company_create(10, 20, 10);
    L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", argv[1],
                lua_tostring(L, -1));
        goto exit;
    }

    company_add(-1);
    company_add(0);
    company_add(1);
    company_add(1);
    company_add(3);
    company_add(0);

    if (company_ref(2) == NULL)
        goto exit;

    /* delete 1, which has 2, 3, 4 as its children */
    company_remove(1);

    /* create the 6th node total, but uses id 1 as it was the first unused */
    company_add(5);

    /* 
     * create 7th node, since next id (which would be 2) can't be cleaned up
     * because there is a reference (from tree_ref) out there, our id will be 3
     */
    company_add(5);
    company_deref(2);

exit:
    lua_close(L);
    company_close();
    return 0;
}
