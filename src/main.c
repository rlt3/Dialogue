#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "dialogue.h"
#include "director.h"
#include "worker.h"
#include "console.h"
#include "arg.h"

void
usage (const char *program)
{
    fprintf(stderr, 
        "Usage: %s [OPTIONS] <STAGE-FILE>\n\n"
        "OPTIONS:\n"
        "   -w <number>\n"
        "       The number of Workers (threads) to spawn. Default is 4.\n\n"
        "   -s\n"
        "       Run the <STAGE-FILE> as a script, spawning no interpreter.\n"
        "       The program will exit when the script finishes. This disables\n"
        "       the `-m' main thread feature.\n\n"
        "   -m\n"
        "       When this is set, the main thread becomes a Worker and counts\n"
        "       towards the `-w` Worker count. If a Lead Actor is set with\n"
        "       thread_id equal to 1, that Actor will be limited to the main\n"
        "       thread.\n\n"
        "   -l\n"
        "       When this flag is set, each Actor must be manually loaded\n"
        "       before it is able to accept messages.\n\n"
        "   -h\n"
        "       Display this help menu and exit the program.\n\n"
        , program);
}

int
main (int argc, char *argv[])
{
    lua_State *L;
    char *file = NULL;
    char *input = NULL;
    char *argv0; /* for ARGBEGIN macro */
    int is_script = 0;
    int ret = 1;

    if (argc == 1) {
        usage(argv[0]);
        goto exit;
    }

    file = argv[argc - 1];

    L = luaL_newstate();

    if (!L) {
        fprintf(stderr, "No memory for the main Lua state!\n");
        goto exit;
    }

    luaL_openlibs(L);

    /* see arg.h */
    ARGBEGIN {
        case 'h': usage(argv[0]); goto exit; break;
        case 's': is_script = 1; break;
        case 'w': dialogue_option_set(WORKER_COUNT, atoi(ARGF())); break;
        case 'm': dialogue_option_set(WORKER_IS_MAIN, 1); break;
        case 'l': dialogue_option_set(ACTOR_AUTO_LOAD, 0); break;
        default: break;
    } ARGEND

    if (is_script)
        dialogue_option_set(WORKER_IS_MAIN, 0);

    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    if (is_script)
        goto load;

    /* from here, the console controls when the program exits */
    //signal(SIGINT, console_handle_interrupt);
    if (console_create() != 0) {
        fprintf(stderr, "Failed to create console thread!");
        goto cleanup;
    }

load:
    if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", file,
                lua_tostring(L, -1));
    }

    if (is_script)
        goto cleanup;

    while (console_is_running()) {
        /* if we transfered 0 actions */
        if (director_transfer_main_actions(L) == 0)
            goto input;

        while (lua_gettop(L) > 0)
            worker_process_action(L);

input:
        if (console_poll_input(&input) == 0)
            console_log("INPUT: %s\n", input);
    }

    console_cleanup();
    ret = 0;
cleanup:
    lua_close(L);
exit:
    return ret;
}
