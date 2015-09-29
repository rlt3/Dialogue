#include "envelope.h"

/*
 * Dialogue = {
 *     "Envelope" : function_new_envelope()
 * }
 */
int
luaopen_Dialogue (lua_State *L)
{
    lua_newtable(L);
    luaopen_Envelope(L); /* leaves function on top of stack */
    lua_setfield(L, -2, "Envelope");
    return 1;
}
