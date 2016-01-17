#include <stdlib.h>
#include "company.h"

/*
 * create(definition_table [, parent object|parent name|parent id])
 */
int
lua_create_actor (lua_State *L)
{
    return 1;
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
    Company *company;
    lua_State *L;

    if (argc == 1)
        usage(argv[0]);

    company = company_create(10);
    L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushlightuserdata(L, company);
    lua_setglobal(L, "__company");

    if (luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", argv[1],
                lua_tostring(L, -1));
        goto exit;
    }

    //lua_pushstring(L, "Foo");
    //company_add_actor(company, L, -1); /* 0 */

    //lua_pushstring(L, "Bar");
    //company_add_actor(company, L, 0);  /* 1 */

    //lua_pushstring(L, "Bar.Foo");
    //company_add_actor(company, L, 1);  /* 2 */

    //lua_pushstring(L, "Bar.Bar");
    //company_add_actor(company, L, 1);  /* 3 */

    //lua_pushstring(L, "Baz");
    //company_add_actor(company, L, 0);  /* 4 */

exit:
    lua_close(L);
    company_close(company);
    return 0;
}
