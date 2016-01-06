#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *
copy_state (int id, lua_State *L)
{
    lua_State *C = lua_newthread(L);
    lua_pushinteger(C, id);
    return C;
}

void
run_state (lua_State *L)
{
    printf("%d\n", (int) lua_tointeger(L, -1));
    lua_pop(L, 1);
}

int
main (void)
{
    const int count = 4;
    int i;
    lua_State *thread[count], *G = luaL_newstate(); 

    for (i = 0; i < count; i++)
        thread[i] = copy_state(i, G);

    for (i = 0; i < count; i++)
        run_state(thread[i]);

    lua_close(G);
    return 0;
}
