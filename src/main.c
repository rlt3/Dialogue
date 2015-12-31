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
    static const char *stack_vars[] = {
        "__one", "__two", "__thr", "__fou", "__fiv", 
        "__six", "__sev", "__eig", "__nin"
    };
    int i;
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

    lua_newtable(L);
    lua_setglobal(L, "__myglobals");

    //luaf(L, "__col = {}");
    //luaf(L, "__col.__index = __col");

    //luaf(L, "function __col:nth(n)"
    //        "   return self[n]    "
    //        "end                  ");

    //luaf(L, "function __col:each(f) "
    //        "    for i = 1, #self do"
    //        "        f(self[i])     "
    //        "    end                "
    //        "end                    ");

    //luaf(L, "function Collection(table)   "
    //        "   setmetatable(table, __col)"
    //        "   return table              "
    //        "end                          ");
    
    //lua_pushstring(L, "Lead");
    //luaf(L, "return %1", 1);

    if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        goto exit;
    }

    for (i = 0; i < 9; i++) {
        //lua_getglobal(L, stack_vars[i]);
        lua_getglobal(L, "__myglobals");
        lua_getfield(L, -1, stack_vars[i]);
        printf("%s => %s\n", stack_vars[i], lua_tostring(L, -1));
        lua_pop(L, 2);
    }

exit:
    lua_close(L);
    return 0;
}
