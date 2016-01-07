#ifndef DIALOGUE_POST
#define DIALOGUE_POST

#define POST_LIB "Dialogue.Post"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

struct Post;

struct Post *
lua_getpost (lua_State *L);

int 
luaopen_Dialogue_Post (lua_State *L);

#endif
