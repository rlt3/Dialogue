#include <stdlib.h>
#include <signal.h>
#include "dialogue.h"
#include "actor.h"
#include "interpreter.h"

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
    const char *script;
    Actor *actor;
    Interpreter *interp;
    lua_State *L;

    signal(SIGINT, handle_sig_int);

    if (argc == 1)
        usage(argv[0]);

    script = argv[1];
    L = luaL_newstate();
    luaL_openlibs(L);

    /* load this module (the one you're reading) into the Actor's state */
    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    if (luaL_loadfile(L, script) || lua_pcall(L, 0, 0, 0))
        fprintf(stderr, "File: %s could not load: %s\n", script, 
                lua_tostring(L, -1));

    interp = interpreter_create(L, &is_running);

    printf("Dialogue v0.0 with Lua v5.2\n"
           "    type `exit()` to exit.\n");

    lua_getglobal(L, "actor");
    actor = lua_check_actor(L, -1);
    lua_pop(L, 1);

    actor_call_action(actor, LOAD);

    while (is_running) {
        actor_call_action(actor, RECEIVE);

        if (interpreter_poll(interp))
            interpreter_lua_interpret(interp, L);
    }

    printf("Goodbye.\n");

    lua_close(L);

    return 0;
}
