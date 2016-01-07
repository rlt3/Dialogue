#ifndef DIALOGUE
#define DIALOGUE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "director.h"

int
luaopen_Dialogue (lua_State *L);

#endif
