#include <stdlib.h>
#include <stdio.h>

#include "dialogue.h"
#include "worker.h"
#include "mailbox.h"

int
main (int argc, char **argv)
{
    int i;
    const int max_boxes = 2;
    Mailbox *boxes[max_boxes];
    lua_State *G = luaL_newstate();
    lua_State *W = lua_newthread(G);
    luaL_openlibs(G);

    for (i = 0; i < max_boxes; i++)
        boxes[i] = mailbox_create(G);

    for (i = 0; i < 1000; i++) {
        lua_newtable(G);
        lua_pushstring(G, "foo");
        lua_rawseti(G, -2, 1);
        mailbox_push_top(G, boxes[0]);
    }

    mailbox_pop_all(W, boxes[0]);

    for (i = 0; i < max_boxes; i++)
        mailbox_destroy(G, boxes[i]);

    lua_close(G);
    return 0;
}
