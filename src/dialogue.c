#include "dialogue.h"
#include "director.h"
#include "actor.h"

int
luaopen_Dialogue (lua_State *L)
{
    lua_newtable(L);

    luaL_requiref(L, "Director", luaopen_Dialogue_Director, 1);
    lua_setfield(L, -2, "Director");

    luaL_requiref(L, "Actor", luaopen_Dialogue_Actor, 1);
    lua_setfield(L, -2, "Actor");

    return 1;
}
