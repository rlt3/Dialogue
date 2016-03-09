#ifndef DIALOGUE
#define DIALOGUE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define DIALOGUE_VERSION     "0.0"
#define DIALOGUE_LUA_VERSION "5.2"

enum DialogueOption {
    WORKER_IS_MAIN, WORKER_COUNT, ACTOR_BASE, ACTOR_MAX, ACTOR_CHILD_MAX,
    ACTOR_AUTO_LOAD
};

void
dialogue_option_set (enum DialogueOption option, int value);

int
luaopen_Dialogue (lua_State *L);

#endif
