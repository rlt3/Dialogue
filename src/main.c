#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "dialogue.h"
#include "luaf.h"

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

    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    luaf(L, "__col = {}");
    luaf(L, "__col.__index = __col");

    luaf(L, "function __col:nth(n)"
            "   return self[n]    "
            "end                  ", 0);

    luaf(L, "function __col:tail() "
            "    local function helper(head, ...) "
            "        return #{...} > 0 and {...} or nil "
            "    end "
            "    return helper((table.unpack or unpack)(self)) "
            "end", 0);

    luaf(L, "function __col:head() "
            "    return table.remove(self, 1) "
            "end", 0);

    luaf(L, "function __col:each(f) "
            "    for i = 1, #self do"
            "        f(self[i])     "
            "    end                "
            "end                    ", 0);

    luaf(L, "function Collection(table)   "
            "   setmetatable(table, __col)"
            "   return table              "
            "end                          ", 0);

    if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        goto exit;
    }

exit:
    lua_close(L);
    return 0;
}
