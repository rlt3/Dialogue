#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "dialogue.h"
#include "collection.h"
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

    luaL_requiref(L, "eval", luaopen_eval, 1);
    luaL_requiref(L, "Collection", luaopen_Collection, 1);
    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 3);

    if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        goto exit;
    }

exit:
    lua_close(L);
    return 0;
}
