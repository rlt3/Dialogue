#include "dialogue.h"

int
lua_dialogue_create (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Creating %s\n", actor);
    return 0;
}

int
lua_dialogue_bench (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Benching %s\n", actor);
    return 0;
}

int
lua_dialogue_join (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Joining %s\n", actor);
    return 0;
}

int
lua_dialogue_send (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Sending %s\n", actor);
    return 0;
}

int
lua_dialogue_load (lua_State *L)
{
    const int actor_arg = 1;
    const char *actor = luaL_checkstring(L, actor_arg);
    printf("Loading %s\n", actor);
    return 0;
}

int
lua_dialogue_error (lua_State *L)
{
    const int error_arg = 1;
    const char *error = luaL_checkstring(L, error_arg);
    printf("%s\n", error);
    return 0;
}

static const luaL_Reg dialogue_actions[] = {
    {"new",   lua_dialogue_create},
    {"bench", lua_dialogue_bench},
    {"join",  lua_dialogue_join},
    {"send",  lua_dialogue_send},
    {"load",  lua_dialogue_load},
    {"error", lua_dialogue_error},
    { NULL, NULL }
};


/*
 * Parse a Line and do what it says. It expects the Line in the form of
 *     Dialogue{ action, actor [, data1 [, ... [, dataN]]] }
 *
 * Because this function is implemented through the __call metamethod, the 
 * first argument on the stack *is* the Dialogue table. Then we push the 
 * first Line element and see if it is a Dialogue action. 
 *
 * If it finds the action, it calls it with the given actor and any other data
 * that may have been sent. If it can't find it, it tries to find the 'error'
 * field in the table and send an error.
 */
int
lua_dialogue_line (lua_State *L)
{
    const int dialogue_table = 1;
    const int line_arg = 2;
    const char *error;
    int i, len, args;

    luaL_checktype(L, line_arg, LUA_TTABLE);
    
    /* -1 because the first element is always an action as a string */
    len  = luaL_len(L, line_arg);
    args = len - 1;

    lua_rawgeti(L, line_arg, 1);
    lua_gettable(L, dialogue_table);

    if (!lua_isfunction(L, -1)) {
        error = "Bad Action!";
        goto error;
    }

    for (i = 2; i <= len; i++)
        lua_rawgeti(L, line_arg, i);

    if (lua_pcall(L, args, 0, 0)) {
        error = lua_tostring(L, -1);
        goto error;
    }

    return 0;

error:
    lua_getfield(L, dialogue_table, "error");
    lua_pushstring(L, error);
    lua_call(L, 1, 0);
    return 0;
}

int
lua_dialogue_quit (lua_State *L)
{
    const int dialogue_table = 1;
    lua_pushstring(L, "__ptr");
    lua_gettable(L, dialogue_table);

    if (lua_isnil(L, -1))
        goto exit;

    printf("GCing...\n");

exit:
    return 0;
}

int
lua_dialogue_tostring (lua_State *L)
{
    lua_pushstring(L, "Dialogue!");
    return 1;
}

static const luaL_Reg dialogue_metamethods[] = {
    {"__call",     lua_dialogue_line},
    {"__gc",       lua_dialogue_quit},
    {"__tostring", lua_dialogue_tostring},
    { NULL, NULL }
};

int
luaopen_Dialogue (lua_State *L)
{
    lua_newtable(L);

    luaL_newmetatable(L, "Dialogue");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, dialogue_metamethods, 0);

    lua_setmetatable(L, -2);
    luaL_setfuncs(L, dialogue_actions, 0);

    return 1;
}
