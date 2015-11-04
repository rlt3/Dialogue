#include "actor.h"
#include "tone.h"

void
audience_set (lua_State *L, Actor *actor)
{
    if (actor == NULL)
        return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, actor->ref);
    lua_rawseti(L, -2, 0);
}

void
audience_children (lua_State *L, Actor *child)
{
    audience_set(L, child);

    if (child->next != NULL)
        audience_children(L, child->next);
}

void
audience_dialogue (lua_State *L, Actor *dialogue)
{
    audience_set(L, dialogue);

    if (dialogue->next != NULL)
        audience_dialogue(L, dialogue->next);

    if (dialogue->child != NULL)
        audience_dialogue(L, dialogue->child);
}

/*
 * Filter an Actor's audience by the tone -- a string.
 */
void
tone_filter (lua_State *L, Actor *actor, const char *tone)
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
    audience_dialogue(L, actor->dialogue);
}

/*
 * Set just the children of the actor as the audience.
 */
void
tone_command (lua_State *L, Actor *actor)
{
    lua_newtable(L);
    audience_children(L, actor->child);
}

/*
 * Set an audience is the actor's parent and the children of that parent.
 */
void
tone_say (lua_State *L, Actor *actor)
{
    lua_newtable(L);
    audience_set(L, actor->parent);
    audience_children(L, actor->parent->child);
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
    audience_set(L, actor);
}
