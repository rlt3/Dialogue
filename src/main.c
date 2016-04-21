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
        "   -a <number>\n"
        "       Set the max number of actors. Default is 64.\n\n"
        "   -w <number>\n"
        "       The number of Workers (threads) to spawn. Default is 4.\n\n"
        "   -s\n"
        "       Run the <STAGE-FILE> as a script, spawning no console.\n"
        "       The program will exit when the script finishes. This disables\n"
        "       the `-m' main thread feature.\n\n"
        "   -m\n"
        "       When this is set, the main thread becomes a Worker and counts\n"
        "       towards the `-w` Worker count. If a Lead Actor is set with\n"
        "       thread_id equal to 1, that Actor will be limited to the main\n"
        "       thread.\n\n"
        "   -l\n"
        "       Loading the Actors manually by calling the method `load' is\n"
        "       required when this flag is set. Normally the `load' method\n"
        "       is called asynchronously when an Actor is created. Note that\n"
        "       you must manually make an asynchronous `load' call via the\n"
        "       method `async' for Actors which require specific threads.\n\n"
        /*
        "   -f\n"
        "       When this flag is set the Actors are forced to be loaded\n"
        "       and destroyed synchronously (rather than asynchronously) and\n"
        "       that the `async' method becomes a redundant synchronous\n"
        "       function. This means Actors will be loaded and destroyed in\n"
        "       whichever thread calls the loading/destruction functions.\n"
        "       This will break Actors which require being loaded/destroyed\n"
        "       in specific threads.\n\n"
        */
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
    int workers = 0;
    int actors = 0;

    if (argc == 1)
        usage(argv[0]);

    ARGBEGIN {
        case 'w': workers = atoi(ARGF()); break;
        case 'a': actors = atoi(ARGF()); break;
    /*
        case 'f': dialogue_option_set(ACTOR_FORCE_SYNC, 1); break;
    */
        case 'l': dialogue_option_set(ACTOR_MANUAL_LOAD, 1); break;
        case 's': is_script = 1; break;
        case 'm': is_worker = 1; break;
        case 'h': usage(argv[0]); break;
        default: break;
    } ARGEND

    /* set this here because we're running this from not in a module */
    dialogue_option_set(ACTOR_CONSOLE_WRITE, 1);

    if (workers > 0) /* atoi errors return 0 */
        dialogue_option_set(WORKER_COUNT, workers);

    if (actors > 0) /* atoi errors return 0 */
        dialogue_option_set(ACTOR_COUNT, actors);

    if (is_script) {
        dialogue_option_set(WORKER_IS_MAIN, 0);
        dialogue_option_set(ACTOR_CONSOLE_WRITE, 0);
        is_worker = 0; /* `-s` overrides `-m` */
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

    path = handle_args(argc, argv);
    file = argv[argc - 1];
    L = luaL_newstate();

    if (!L) {
        fprintf(stderr, "No memory for the main Lua state!\n");
        goto exit;
    }

    luaL_openlibs(L);
    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    switch (path) {
    case MAIN_SCRIPT:
        if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
            fprintf(stderr, "%s: %s\n", file, lua_tostring(L, -1));
            lua_close(L);
            goto exit;
        } else {
            lua_close(L);
        }
        break;

    case MAIN_WORKER:
        if (console_start(L, file, CONSOLE_THREADED) != 0) {
            fprintf(stderr, "Failed to start console's thread!\n");
            goto exit;
        }
        director_process_work();
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
