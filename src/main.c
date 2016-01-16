#include "company.h"

int
main (int argc, char **argv)
{
    Company *company = company_create(10);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushstring(L, "Foo");
    company_add_actor(company, L, -1); /* 0 */

    lua_pushstring(L, "Bar");
    company_add_actor(company, L, 0);  /* 1 */

    lua_pushstring(L, "Bar.Foo");
    company_add_actor(company, L, 1);  /* 2 */

    lua_pushstring(L, "Bar.Bar");
    company_add_actor(company, L, 1);  /* 3 */

    lua_pushstring(L, "Baz");
    company_add_actor(company, L, 0);  /* 4 */

    lua_close(L);
    company_close(company);
    return 0;
}
