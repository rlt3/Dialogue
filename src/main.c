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

/*
 * Call Lua from C by passing code as a string. Ret args is the number of items
 * left on the stack after interpreting.
 */
void
lua_interpret (lua_State *L, const char *input, int ret_args)
{
    if (input == NULL)
        return;

    lua_getglobal(L, "loadstring");
    lua_pushstring(L, input);
    lua_call(L, 1, 1);
    
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, ret_args, 0))
            printf("%s\n", lua_tostring(L, -1));
    } else {
        lua_pop(L, 1);
    }
}

/*
 * Lua Format.
 *
 * Call Lua from C by passing a const *char array with code. Optionally leave
 * items on the stack by preprending 'return' to your code and passing the
 * number of items to be left on the stack.
 *
 * Looks for stack variables in the form of %[1-9], e.g. "%2.__index = %1". 
 * Only supports 9 stack items, 1 through 9.
 *
 * luaf returns the number of arguments left on the stack.
 *
 * Examples:
 *
 * luaf(L, "return %1", 1);
 * luaf(L, "return %1:method_with_error()", 2);
 * luaf(L, "%1:each(function(e) %2(%3) end)");
 */
int
luaf (lua_State *L, const char *format, ...)
{
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

    va_list args;
    int ret_args = 0;
    
    char code[1024] = {0};
    int processed[9] = {0};
    int last_index = 0;
    int stack_index;
    int index;

    /* we can't do va_args unless we *know* we have them */
    if (strlen(format) > 7) {
        if (strncmp(format, "return ", 7) == 0) {
            va_start(args, format);
            ret_args += va_arg(args, int);
            va_end(args);
        }
    }

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
    lua_interpret(L, code, ret_args);

    return ret_args;
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

    luaf(L, "__col = {}");
    luaf(L, "__col.__index = __col");

    luaf(L, "function __col:nth(n)"
            "   return self[n]    "
            "end                  ");

    luaf(L, "function __col:each(f) "
            "    for i = 1, #self do"
            "        f(self[i])     "
            "    end                "
            "end                    ");

    luaf(L, "function Collection(table)   "
            "   setmetatable(table, __col)"
            "   return table              "
            "end                          ");

    /* create & leave a Collection on top of the stack */
    luaf(L, "return Collection{5, 6, 7}", 1);
    
    /* using the first item on the stack, call the each method */
    luaf(L, "%1:each(print)");

    lua_close(L);
    return 0;
}
