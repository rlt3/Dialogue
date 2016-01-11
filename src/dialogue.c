#include "dialogue.h"
#include "director.h"

int
luaopen_Dialogue (lua_State *L)
{
    lua_newtable(L);
    luaL_requiref(L, "Director", luaopen_Dialogue_Director, 1);
    lua_setfield(L, -2, "Director");
    return 1;
}
