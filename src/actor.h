#ifndef DIALOGUE_ACTOR
#define DIALOGUE_ACTOR

#define ACTOR_LIB "Dialogue.Actor"

/*
 * Each Actor has a completely separate lua_State. The biggest reason why is 
 * that this enables concurrent message between Actors (though an Actor only
 * can receive messages one at a time).
 *
 * An actor holds a table (to be used as an array) of Scripts that it 'reads'
 * from to produce messages.
 */

typedef struct Actor {
    int script_table_reference;
    lua_State *L;
    pthread_mutex_t mutex;
} Actor;

/*
 * Actor() => actor
 * Create a new Actor and return it.
 */
int
lua_actor_new (lua_State *L);

#endif
