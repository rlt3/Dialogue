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
    lua_newtable(L);

    luaopen_Envelope(L);
    lua_setfield(L, -2, "Envelope");

    luaopen_Script(L);
    lua_setfield(L, -2, "Script");

    return 1;
}
