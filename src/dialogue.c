#include <stdio.h>
#include "envelope.h"
#include "script.h"

/*
 * Dialogue = {
 *     "Envelope" : function_new_envelope()
 *     "Script" : function_new_script()
 * }
 */
int
luaopen_Dialogue (lua_State *L)
{
    lua_pushboolean(L, 1);
    return 1;
}
