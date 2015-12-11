#include <stdlib.h>
#include <signal.h>
#include "dialogue.h"
#include "actor.h"
#include "interpreter.h"
#include "utils.h"

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

void
lead_actors_receive_update (lua_State *L)
{
    Actor *actor;
    int table_index = actor_lead_table(L);

    lua_pushnil(L);
    while (lua_next(L, table_index)) {
        actor = lua_check_actor(L, -1);
        actor_call_action(actor, RECEIVE);

        utils_push_objref_method(L, actor->ref, "send");
        lua_newtable(L);
        lua_pushstring(L, "main");
        lua_rawseti(L, -2, 1);
        lua_call(L, 2, 0);
        lua_pop(L, 2);
    }
    lua_pop(L, 1);
}

int
main (int argc, char **argv)
{
    const char *script;
    Interpreter *interp;
    lua_State *L;

    signal(SIGINT, handle_sig_int);

    if (argc == 1)
        usage(argv[0]);

    script = argv[1];
    L = luaL_newstate();
    luaL_openlibs(L);
    
    lua_pushboolean(L, 1);
    lua_setglobal(L, "__main_thread");

    /* load this module (the one you're reading) into the Actor's state */
    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    if (luaL_loadfile(L, script) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "File: %s could not load: %s\n", script,
                lua_tostring(L, -1));
        goto exit;
    }
    
    interp = interpreter_create(L, &is_running);
    while (is_running) {
        lead_actors_receive_update (L);
        if (interpreter_poll(interp))
            interpreter_lua_interpret(interp, L);
    }

exit:
    lua_close(L);
    return 0;
}
