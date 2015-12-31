#include "luaf.h"

/*
 * Call Lua from C by passing code as a string. Pass the number of expected
 * items to be left on top of the stack.
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
        "__one", "__two", "__thr", "__fou", "__fiv", 
        "__six", "__sev", "__eig", "__nin"
    };

    va_list args;
    int ret_args = 0;
    
    char code[1024] = {0};
    int processed[8] = {0};
    int last_index = 0;
    int ref, depth, index, i, j;

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
            if (!processed[index]) {
                luaL_checkany(L, index);

                /* bubble the element to the top */
                depth = lua_gettop(L) - index;
                for (j = 0; j < depth; j++)
                    lua_insert(L, index);

                lua_setglobal(L, stack_vars[index]);
                processed[index] = 1;
            }

            /* copy the stack variable we've found */
            strcat(code, stack_vars[index]);

            /* push past the %[1-9] */
            i += 2;
            last_index = i;
        }
    }

    strncat(code, format + last_index, i);
    lua_interpret(L, code, ret_args);

    return ret_args;
}
