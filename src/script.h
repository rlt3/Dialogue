#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>

#define SCRIPT_LIB "Dialogue.Actor.Script"

typedef struct Script {
    struct Script *next;
    struct Actor *actor;

    pthread_mutex_t state_mutex;

    int is_loaded;

    int table_ref;
    int object_ref;
    int ref;
} Script;

/*
 * Check for a Script at index. Errors if it isn't a Script.
 */
Script *
lua_check_script (lua_State *L, int index);

int 
luaopen_Dialogue_Actor_Script (lua_State *L);

#endif
