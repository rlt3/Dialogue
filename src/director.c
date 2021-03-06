#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "director.h"
#include "console.h"
#include "worker.h"
#include "utils.h"

typedef struct Director {
    Worker **workers;
    int worker_count;
    int rand_seed;
    struct timeval start;
    struct timeval now;
} Director;

static Director *global_director = NULL;

/*
 * Create the Director and N workers where N is `workers`. Each worker is 
 * allocated a mailbox and the Director itself has a mailbox for the main
 * thread.
 *
 * Returns 0 if successfuly, >0 if the system didn't have enough memory.
 */
int
director_create (const int has_main, const int num_workers)
{
    int i, start = 0, ret = 1;

    global_director = malloc(sizeof(*global_director));

    if (!global_director)
        goto exit;

    global_director->workers = malloc(sizeof(Worker*) * num_workers);

    if (!global_director->workers) {
        free(global_director);
        goto exit;
    }

    /* Setup a Lua state just used for its stack which acts like a mailbox */
    if (has_main) {
        /* the main thread doesn't need a thread `started`, so skip it */
        global_director->workers[0] = worker_create(0);
        start = 1;
    }

    global_director->worker_count = num_workers;

    /* set memory to NULL so if an error occurs, NULL checks will catch */
    for (i = start; i < global_director->worker_count; i++)
        global_director->workers[i] = NULL;

    for (i = start; i < global_director->worker_count; i++) {
        global_director->workers[i] = worker_start(i);

        if (!global_director->workers[i]) {
            director_close();
            goto exit;
        }
    }

    global_director->rand_seed = time(NULL);
    srand(global_director->rand_seed);

    gettimeofday(&global_director->start, NULL);
    global_director->now = global_director->start;

    ret = 0;
exit:
    return ret;
}

/*
 * Set the `director_action` function inside the Lua state as "Director".
 */
void
director_set (lua_State *L)
{
    lua_pushcfunction(L, director_take_action);
    lua_setglobal(L, "Director");
}

/*
 * The Director takes (and pops) whatever is on top of L and gives it to a
 * Worker and does no validation. The validation occurs at the Worker level,
 * where other errors might pop up from bad inputs.
 *
 * From a random starting point (generated by the seed) 0..worker-max, we loop
 * through the workers until one isn't busy. Send the action to the open
 * worker.
 *
 * An optional thread_id can be passed which tells the director which Worker
 * process to target. If the thread_id == 1, the Director handles the message
 * itself on the main thread.
 *
 * Returns 0 if successful.
 */
int
director_take_action (lua_State *L)
{
    const int thread_arg = 2;
    int count, start, i;
    int thread = -1;
    int ret = 1;

    /* luaL_optint will return `-1` even if args == 2 and arg @ 2 is not an
     * integer. This means we *must* always check the number of args to make
     * sure the stack is balanced.
     */
    if (lua_gettop(L) == thread_arg) {
        thread = luaL_optint(L, thread_arg, -1);
        lua_pop(L, 1);
    }

    /* block & wait for the specific Worker (thread) to open up */
    if (thread > -1 && thread < global_director->worker_count + 1) {
        if (worker_give_action(global_director->workers[thread - 1], L) != 0)
            goto error;
        goto exit;
    }

    count = global_director->worker_count;
    start = rand() % count;

    /* loop forever until a Worker is available to take an Action */
    for (i = start; i < count; i = (i + 1) % count)
        if (worker_take_action(global_director->workers[i], L) == 0)
            break;

exit:
    ret = 0;
error:
    lua_pop(L, 1); /* the action */
    return ret;
}

/*
 * This function blocks and becomes a Worker thread using the first Worker in
 * the Director's worker list. This function should only be called when
 * `has_main` passed to `director_create` was true.
 */
void
director_process_work ()
{
    worker_thread(global_director->workers[0]);
}

/*
 * Close the Director and all of the Workers.
 */
void
director_close ()
{
    int i;

    /* stop everything first */
    for (i = 0; i < global_director->worker_count; i++)
        worker_stop(global_director->workers[i]);

    /* then cleanup, it avoids a lot of problems */
    for (i = 0; i < global_director->worker_count; i++)
        worker_cleanup(global_director->workers[i]);

    free(global_director->workers);
    free(global_director);
}
