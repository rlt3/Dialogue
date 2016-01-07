#include "worker.h"

struct Worker {
    lua_State *L;
};

int
lua_worker_take_top (lua_State *L, Worker *worker)
{
    return 1;
}

/*
 * Because this function is implemented through the __call metamethod, the 
 * first argument on the stack *is* the Dialogue table. Then we push the 
 * first Line element and see if it is a Dialogue action. 
 *
 * If it finds the action, it calls it with the given actor and any other data
 * that may have been sent. If it can't find it, it tries to find the 'error'
 * field in the table and send an error.
 */
//int
//lua_worker_take_top (lua_State *L, Worker *worker)
//{
//    const int action_arg = -1;
//    const char *error;
//    int i, len, args;
//
//    luaL_checktype(L, action_arg, LUA_TTABLE);
//    
//    /* -1 because the first element is always an action as a string */
//    len  = luaL_len(L, action_arg);
//    args = len - 1;
//
//    lua_rawgeti(L, action_arg, 1);
//    lua_gettable(L, dialogue_table);
//
//    if (!lua_isfunction(L, -1)) {
//        error = "Bad Action!";
//        goto error;
//    }
//
//    for (i = 2; i <= len; i++)
//        lua_rawgeti(L, action_arg, i);
//
//    if (lua_pcall(L, args, 0, 0)) {
//        error = lua_tostring(L, -1);
//        goto error;
//    }
//
//    return 0;
//
//error:
//    lua_getfield(L, dialogue_table, "error");
//    lua_pushstring(L, error);
//    lua_call(L, 1, 0);
//    return 0;
//}

