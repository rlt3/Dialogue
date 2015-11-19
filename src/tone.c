#include <stdio.h>
#include "actor.h"
#include "tone.h"
#include "utils.h"

/*
 * Since we're doing this recursively, we pass in the accumulator (acc) to each
 * function and make sure to increment as we do so.
 */

void
audience_set (lua_State *L, Actor *actor, int acc)
{
    if (actor == NULL)
        return;

    utils_push_object(L, actor, ACTOR_LIB);
    lua_rawseti(L, -2, acc);
}

void
audience_siblings (lua_State *L, Actor *child, int acc)
{
    audience_set(L, child, acc);

    if (child->next != NULL)
        audience_siblings(L, child->next, acc + 1);
}

/*
 * From a parent actor, go through each child and set them in the table. If 
 * that child has children itself, call this function on it and use the return
 * value as the next index in the for loop. This will recursively fill out the
 * tree in a top-down manner. 
 */
int
audience_dialogue (lua_State *L, Actor *parent, int acc)
{
    Actor *child;
    audience_set(L, parent, acc);

    printf("audience_dialogue: %p, %d\n", parent, acc);

    for (child = parent->child, acc++; child != NULL; child = child->next) {
        if (child->child != NULL) {
            acc = audience_dialogue(L, child, acc);
        } else {
            audience_set(L, child, acc);
            acc++;
        }
    }

    return acc;
}

/*
 * Filter an Actor's audience by the tone -- a string.
 */
void
audience_filter_tone (lua_State *L, Actor *actor, const char *tone)
{
    if (tone == NULL)
        luaL_error(L, "Tone cannot be empty!");

    /* a vicious hack: each of our tones has a different first character */
    switch(tone[0]) {
    case 's':
        tone_say(L, actor);
        break;

    case 'c':
        tone_command(L, actor);
        break;

    case 'y':
        tone_yell(L, actor);
        break;

    case 'w':
        break;

    case 't':
        tone_think(L, actor);
        break;

    default:
        luaL_error(L, "%s is not a valid tone", tone);
        break;
    }
}

/*
 * Set the entire dialogue tree as the audience.
 */
void
tone_yell (lua_State *L, Actor *actor)
{
    lua_newtable(L);
    audience_dialogue(L, actor->dialogue, 1);
}

/*
 * Set just the children of the actor as the audience.
 */
void
tone_command (lua_State *L, Actor *actor)
{
    lua_newtable(L);
    audience_siblings(L, actor->child, 1);
}

/*
 * Set an audience is the actor's parent and the children of that parent.
 */
void
tone_say (lua_State *L, Actor *actor)
{
    lua_newtable(L);
    audience_set(L, actor->parent, 1);
    audience_siblings(L, actor->parent->child, 2);
}

/*
 * Set a specific actor as the audience.
 */
void
tone_whisper (lua_State *L, Actor *actor)
{
}

/*
 * Set itself as its audience.
 */
void
tone_think (lua_State *L, Actor *actor)
{
    lua_newtable(L);
    audience_set(L, actor, 1);
}
