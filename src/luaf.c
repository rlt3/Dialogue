#include "luaf.h"

/*
 * Load our string with the global environment.
 */
int
lua_eval (lua_State *L)
{
    const char *code = luaL_checkstring(L, 1);
    const int ret_args = luaL_checkinteger(L, 2);

    lua_getglobal(L, "load");
    lua_pushstring(L, code);
    lua_pushnil(L);
    lua_pushstring(L, "t");
    lua_getglobal(L, "_G");
    lua_call(L, 4, 1);
    
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, ret_args, 0))
            printf("%s\n", lua_tostring(L, -1));
    } else {
        lua_pop(L, 1);
    }

    return ret_args;
}

/*
 * Lua Format.
 *
 * Call Lua from C by passing a format string of code. Optionally leave items
 * on the stack by preprending 'return' to your code and passing the number of
 * items to be left on the stack.
 *
 * Looks for stack variables in the form of %[1-9], e.g. "%2.__index = %1".
 * Only supports 8 stack items, 1 through 9.
 *
 * luaf returns the number of arguments left on the stack.
 *
 * Examples:
 *
 * luaf(L, "return %1", 1);
 * luaf(L, "return %1:method_with_error()", 2);
 * luaf(L, "%1:each(function(e) %2(e) end)");
 */
int
luaf (lua_State *L, const char *format, ...)
{
    static const char *stack_vars[] = {
        "__a", "__b", "__c", "__d", "__e", "__f", "__g", "__h", "__i"
    };

    va_list args;
    int ret_args = 0;
    
    char code[1024] = {0};
    int processed[8] = {0};
    int last_index = 0;
    int depth, index, i, j;

    /* we can't do va_args unless we *know* we have them */
    if (strlen(format) > 7) {
        if (strncmp(format, "return ", 7) == 0) {
            va_start(args, format);
            ret_args += va_arg(args, int);
            va_end(args);
        }
    }

    for (i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            /* copy the last bit of the string we've ran through */
            if (i != 0)
                strncat(code, format + last_index, i - last_index);

            /* convert an ascii number into an integer and then zero offset */
            index = (format[i + 1] - '0');

            /* check and load the stack variable into the environment */
            if (!processed[index - 1]) {
                luaL_checkany(L, index);
                lua_pushvalue(L, index);
                lua_setglobal(L, stack_vars[index - 1]);
                processed[index - 1] = 1;
            }

            /* copy the stack variable we've found */
            strcat(code, stack_vars[index - 1]);

            /* push past the %[1-9] */
            i += 2;
            last_index = i;
        }
    }

    strncat(code, format + last_index, i);

    lua_getglobal(L, "eval");
    lua_pushstring(L, code);
    lua_pushinteger(L, ret_args);
    lua_call(L, 2, ret_args);

    return ret_args;
}
