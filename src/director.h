#ifndef DIALOGUE_DIRECTOR
#define DIALOGUE_DIRECTOR

#define POST_LIB "Dialogue.Director"

#include "dialogue.h"

typedef struct Director Director;

int
lua_director_action (lua_State *L);

int
lua_director_quit (lua_State *L);

int
lua_director_tostring (lua_State *L);

static const luaL_Reg director_metamethods[] = {
    {"__call",     lua_director_action},
    {"__gc",       lua_director_quit},
    {"__tostring", lua_director_tostring},
    { NULL, NULL }
};

#endif
