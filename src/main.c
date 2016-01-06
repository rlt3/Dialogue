#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#include "dialogue.h"
#include "interpreter.h"

void
usage (const char *program)
{
    fprintf(stderr, "%s [script]\n", program);
    exit(1);
}

int
main (int argc, char **argv)
{
    struct timeval stop, start;
    short int running = 1;
    const char *file;
    lua_State *L;

    if (argc == 1)
        usage(argv[0]);

    file = argv[1];
    L = luaL_newstate();
    luaL_openlibs(L);

    luaL_requiref(L, "Dialogue", luaopen_Dialogue, 1);
    lua_pop(L, 1);

    //interpreter_register(L, &running);

    /*
     * from http://stackoverflow.com/questions/10192903/time-in-milliseconds
     */
    gettimeofday(&start, NULL);

    if (luaL_loadfile(L, file) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        interpreter_exit();
        goto exit;
    }

   // while (running)
   //     lua_interpret(L);

    gettimeofday(&stop, NULL);
    printf("%f\n", (double)(stop.tv_usec - start.tv_usec) / 1000000 
                   + (double)(stop.tv_sec - start.tv_sec));

exit:
    lua_close(L);
    return 0;
}
