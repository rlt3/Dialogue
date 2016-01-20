#include <stdlib.h>
#include "company.h"
#include "company_meta.h"

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

    luaL_requiref(L, "Actor", luaopen_Dialogue_Company, 1);
    company_set_table(L, company);
    lua_pop(L, 1);

    if (luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", argv[1],
                lua_tostring(L, -1));
        goto exit;
    }

exit:
    lua_close(L);
    company_close(company);
    return 0;
}
