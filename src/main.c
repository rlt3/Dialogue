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
    exit(1);
}

enum ExecutionPath {
    MAIN_SCRIPT, MAIN_WORKER, MAIN_CONSOLE
};

/*
 * Because our main thread can be so many different things depending on the
 * options, we just switch on an ExecutionPath between the different ways the
 * main thread can function.
 */
enum ExecutionPath
handle_args (int argc, char *argv[])
{
    enum ExecutionPath path = MAIN_CONSOLE;
    char *argv0; /* for ARGBEGIN macro, see arg.h */
    int is_script = 0;
    int is_worker = 0;

    ARGBEGIN {
        case 'w': dialogue_option_set(WORKER_COUNT, atoi(ARGF())); break;
        case 'l': dialogue_option_set(ACTOR_AUTO_LOAD, 0); break;
        case 's': is_script = 1; break;
        case 'm': is_worker = 1; break;
        case 'h': usage(argv[0]); break;
        default: break;
    } ARGEND

    /* `-s` overrides `-m` */
    if (is_script) {
        dialogue_option_set(WORKER_IS_MAIN, 0);
        dialogue_option_set(ACTOR_CONSOLE_WRITE, 0);
        is_worker = 0;
        path = MAIN_SCRIPT;
    }

    if (is_worker) {
        dialogue_option_set(WORKER_IS_MAIN, 1);
        path = MAIN_WORKER;
    }

    return path;
}

int
main (int argc, char *argv[])
{
    lua_State *L = NULL;
    char *file = NULL;
    int ret = 1;
    enum ExecutionPath path;

    if (argc == 1)
        usage(argv[0]);

    file = argv[argc - 1];
    path = handle_args(argc, argv);
    L = luaL_newstate();

    if (!L) {
        fprintf(stderr, "No memory for the main Lua state!\n");
        goto exit;
    }

    luaL_openlibs(L);
    dialogue_set_io_write(L);
    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    switch (path) {
    case MAIN_SCRIPT:
        if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
            fprintf(stderr, "File: %s could not load: %s\n", file,
                    lua_tostring(L, -1));
            lua_close(L);
            goto exit;
        } else {
            lua_close(L);
        }
        break;

    case MAIN_WORKER:
        if (console_start(L, file, CONSOLE_THREADED) != 0)
            goto exit;
        //director_process_work();
        wait_for_console_exit();
        break;

    case MAIN_CONSOLE:
        console_start(L, file, CONSOLE_NON_THREADED);
        break;

    default:
        break;
    }

    ret = 0;
exit:
    return ret;
}
