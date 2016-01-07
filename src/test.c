#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int
lua_dialogue_create (lua_State *L)
{
    printf("Create.\n");
    return 0;
}

int
lua_dialogue_bench (lua_State *L)
{
    printf("Bench.\n");
    return 0;
}

int
lua_dialogue_join (lua_State *L)
{
    printf("Join.\n");
    return 0;
}

int
lua_dialogue_send (lua_State *L)
{
    printf("Send.\n");
    return 0;
}

int
lua_dialogue_load (lua_State *L)
{
    printf("Load.\n");
    return 0;
}

int
lua_dialogue_error (lua_State *L)
{
    printf("Load.\n");
    return 0;
}

/*
 * Parse a Letter and do what it says. It expects a Letter in the form of:
 *     Dialogue{ action, actor [, data1 [, ... [, dataN]]] }
 *
 * It looks for 'Dialogue' in the global table and then for the field 'action'
 * in that table. If it can't find it, it tries to find the 'error' field in
 * the table and send an error.
 *
 * If it finds the action, it calls it with the given actor and any other data
 * that may have been sent.
 */
int
lua_dialogue_action (lua_State *L)
{
    const int letter_arg = 1;
    const int dialogue_table = 2;
    const char *error;
    int i, args;

    luaL_checktype(L, letter_arg, LUA_TTABLE);
    
    /* -1 because the first element is always an action as a string */
    args = luaL_len(L, letter_arg) - 1;

    /* see if 'send' exists first */
    lua_getglobal(L, "Dialogue");

    if (lua_isnil(L, -1));
        printf("%s\n", lua_tostring(L, -1));

    //lua_rawgeti(L, letter_arg, 1);
    //lua_gettable(L, dialogue_table);

    //if (!lua_isfunction(L, -1)) {
    //    error = "Bad Action!";
    //    goto error;
    //}

    //for (i = 2; i <= args; i++)
    //    lua_rawgeti(L, letter_arg, i);

    //if (lua_pcall(L, args, 0, 0)) {
    //    error = lua_tostring(L, -1);
    //    goto error;
    //}

    return 0;

error:
    printf("%s\n", error);
    return 0;
}

int
lua_dialogue_quit (lua_State *L)
{
    return 0;
}

int
lua_dialogue_tostring (lua_State *L)
{
    lua_pushstring(L, "Dialogue!");
    return 1;
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

static const luaL_Reg dialogue_metamethods[] = {
    {"__call",     lua_dialogue_action},
    {"__gc",       lua_dialogue_quit},
    {"__tostring", lua_dialogue_tostring},
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
