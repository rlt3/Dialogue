#include "actor.h"

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
 * Set itself as its audience.
 */
void
tone_think (lua_State *L, Actor *actor)
{
    lua_newtable(L);
    audience_set(L, actor);
}
