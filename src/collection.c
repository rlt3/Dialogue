#include "collection.h"

int
luaopen_Dialogue_Collection (lua_State *L)
{
    luaf(L, "__col = {}");
    luaf(L, "__col.__index = __col");

    luaf(L, "function __col:nth(n)"
            "   return self[n]    "
            "end                  ", 0);

    luaf(L, "function __col:tail() "
            "    local function helper(head, ...) "
            "        return #{...} > 0 and {...} or nil "
            "    end "
            "    return helper((table.unpack or unpack)(self)) "
            "end", 0);

    luaf(L, "function __col:head() "
            "    return table.remove(self, 1) "
            "end", 0);

    luaf(L, "function __col:each(f) "
            "    for i = 1, #self do"
            "        f(self[i])     "
            "    end                "
            "end                    ", 0);

    luaf(L, "function Collection(table)   "
            "   setmetatable(table, __col)"
            "   return table              "
            "end                          ", 0);

    lua_getglobal(L, "Collection");
    return 1;

    //return luaf(L, "return Collection", 1);
}
