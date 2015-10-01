#include <stdio.h>
#include "envelope.h"
#include "script.h"

/*
 * Dialogue = require 'Dialogue'
 * Dialogue.Envelope
 * Dialogue.Actor.Script
 */
int
luaopen_Dialogue (lua_State *L)
{
    lua_pushboolean(L, 1);
    return 1;
}
