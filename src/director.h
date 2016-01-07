#ifndef DIALOGUE_DIRECTOR
#define DIALOGUE_DIRECTOR

#define POST_LIB "Dialogue.Director"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct Director Director;

/*
 * Returns the Director of the Dialogue in the given Lua state. Initializes it
 * if not done already.
 */
Director *
director_or_init (lua_State *L);

int 
luaopen_Dialogue_Director (lua_State *L);

#endif
