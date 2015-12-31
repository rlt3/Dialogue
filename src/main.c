#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

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
    int ret_args = 0;

    if (input == NULL)
        return;

    /*
     * Do vargs for count of how many items to leave on stack.
     * lua_interpret(L, "return 5, 8", 2); => leave 5, 8 on stack.
     */

    if (strlen(input) > 7)
        if (strncmp(input, "return ", 7) == 0)
            ret_args = 1;

    lua_getglobal(L, "loadstring");
    lua_pushstring(L, input);
    lua_call(L, 1, 1);
    
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, ret_args, 0))
            lua_printerror(L);
    } else {
        lua_pop(L, 1);
    }
}

static const char *stack_vars[] = {
    "__one",
    "__two",
    "__thr",
    "__fou",
    "__fiv",
    "__six",
    "__sev",
    "__eig",
    "__nin"
};

/*
 * Accepts Lua code where variables may be written %[0-9] -- %1, %6 -- to 
 * represent themselves on the stack.
 */
int
luaf (lua_State *L, const char *format)
{
    char code[1024] = {0};
    int processed[9] = {0};
    int last_index = 0;
    int stack_index;
    int index;

    for (index = 0; format[index] != '\0'; index++) {
        if (format[index] == '%') {
            /* copy the last bit of the string we've ran through */
            if (index != 0)
                strncat(code, format + last_index, index - last_index - 1);

            stack_index = format[index + 1] - '0';

            /* check and load the stack variable into the environment */
            if (!processed[stack_index]) {
                luaL_checkany(L, stack_index);
                lua_pushvalue(L, stack_index);
                lua_setglobal(L, stack_vars[stack_index - 1]);
                processed[stack_index] = 1;
            }

            /* copy the stack variable we've found */
            strcat(code, stack_vars[stack_index - 1]);

            /* push past the %[0-9] */
            index += 2;
            last_index = index;
        }
    }

    strncat(code, format + last_index, index);
    printf("%s\n", code);
    lua_interpret(L, code);

    return 0;
}

int
print_collection (lua_State *L)
{
    //luaL_checktype(L, 1, LUA_TTABLE);
    //lua_pushvalue(L, 1);
    //lua_setglobal(L, "fun");
    //lua_interpret(L, "fun:each(function(e) print(e) end)");

    luaf(L, "%1:each(function(e) print(e) end)");

    return 0;
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

    ///* load this module (the one you're reading) into the Actor's state */
    ////luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    ////lua_pop(L, 1);

    lua_pushcfunction(L, print_collection);
    lua_setglobal(L, "print_collection");

    if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        goto exit;
    }

    lua_interpret(L, "return #t");
    printf("%d\n", (int) luaL_checkinteger(L, -1));

exit:
    lua_close(L);
    return 0;
}
