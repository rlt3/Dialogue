#include "director.h"

struct Director {
    Worker **workers;
    Mailbox *mailbox;
    int worker_count;
    int rand_seed;
};

/*
 * Returns the Director of the Dialogue in the given Lua state. Initializes it
 * if not done already.
 */
Director *
director_or_init (lua_State *L)
{
    const int dialogue_table = 1;
}
