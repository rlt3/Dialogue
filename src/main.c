#include "company.h"

int
main (int argc, char **argv)
{
    Company *company = company_create(10);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    lua_close(L);
    company_close(company);
    return 0;
}
