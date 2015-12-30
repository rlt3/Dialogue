#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static short int is_running = 1;

void 
handle_sig_int (int arg)
{
    is_running = 0;
}

void
usage (const char *program)
{
    fprintf(stderr, "%s [script]\n", program);
    exit(1);
}

static int
utils_collection_nth (lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_rawgeti(L, 1, luaL_checkint(L, 2));
    puts("nth");
    return 1;
}

static int
utils_collection_each (lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    puts("each");
    return 1;
}

static int
utils_collection_new (lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_getmetatable(L, "Utils.Collection");
    lua_setmetatable(L, -2);
    puts("make");
    return 1;
}

static const luaL_Reg collection_methods[] = {
    {"each", utils_collection_each},
    {"nth", utils_collection_nth}
};

int
luaopen_Utils_Collection (lua_State *L)
{
    luaL_newmetatable(L, "Utils.Collection");

    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");

    luaL_setfuncs(L, collection_methods, 0);

    lua_newtable(L);
    lua_pushcfunction(L, utils_collection_new);
    lua_setfield(L, -2, "new");

    return 1;
}

/*
 * Print the Lua error.
 */
void
lua_printerror (lua_State *L)
{
    printf("%s\n", lua_tostring(L, -1));
}

/*
 * Interpret the input in the given lua_State*.
 */
void
lua_interpret (lua_State *L, const char *input)
{
    if (input == NULL)
        return;

    lua_getglobal(L, "loadstring");
    lua_pushstring(L, input);
    lua_call(L, 1, 1);
    
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0))
            lua_printerror(L);
    } else {
        lua_pop(L, 1);
    }
}

//int
//vluaf (lua_State *L, const char *format, va_list args)
//{
//    for (; *format != 0; ++format) {
//        if (*format == '%') {
//            ++format;
//
//            if (*format == '1') {
//            }
//        }
//    }
//}

int
luaf (lua_State *L, const char *format, ...)
{
   va_list arg;
   int rc;

   va_start (arg, format);
   rc = vluaf(L, format, arg);
   va_end (arg);

   printf("\n");

   return done;
}

int
main (int argc, char **argv)
{
    const char *file;
    lua_State *L;

    signal(SIGINT, handle_sig_int);

    if (argc == 1)
        usage(argv[0]);

    file = argv[1];
    L = luaL_newstate();
    luaL_openlibs(L);

    /* load this module (the one you're reading) into the Actor's state */
    //luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    //lua_pop(L, 1);

    if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        goto exit;
    }

    lua_interpret(L, "t:each(function(e) print(e) end)");

exit:
    lua_close(L);
    return 0;
}
