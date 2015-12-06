#ifndef DIALOGUE_SCRIPT
#define DIALOGUE_SCRIPT

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define SCRIPT_LIB "Dialogue.Actor.Script"

typedef struct Script {
    struct Script *next;
    struct Actor *actor;

    int is_loaded;

    int table_ref;
    int object_ref;
    int ref;
} Script;


#endif
