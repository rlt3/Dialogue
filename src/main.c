#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "dialogue.h"
#include "director.h"
#include "worker.h"
#include "console.h"

void
usage (const char *program)
{
    fprintf(stderr, "%s [script]\n", program);
    exit(1);
}

int
main (int argc, char **argv)
{
    lua_State *L;
    char *line = NULL;
    int ret = 1;

    signal(SIGINT, console_handle_interrupt);

    if (argc == 1)
        usage(argv[0]);

    L = luaL_newstate();

    if (!L) {
        fprintf(stderr, "No memory for the main Lua state!\n");
        goto exit;
    }

    luaL_openlibs(L);

    dialogue_option_set(WORKER_IS_MAIN, 1);
    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    if (luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", argv[1],
                lua_tostring(L, -1));
        goto cleanup;
    }

    /* from here, the console controls when the program exits */
    if (console_create() != 0) {
        fprintf(stderr, "Failed to create console thread!");
        goto cleanup;
    }

    while (console_is_running()) {
        /* if we transfered 0 actions */
        if (director_transfer_main_actions(L) == 0)
            goto input;

        while (lua_gettop(L) > 0)
            worker_process_action(L);

input:
        if (console_poll_input(&line) == 0)
            printf("INPUT: %s\n", line);
    }

    console_cleanup();
    ret = 0;
cleanup:
    lua_close(L);
exit:
    return ret;
}
