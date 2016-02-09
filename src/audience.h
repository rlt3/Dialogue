#ifndef DIALOGUE_AUDIENCE
#define DIALOGUE_AUDIENCE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

struct Actor;

void
audience_set (lua_State *L, Actor *actor, int acc);

/*
 * Filter an Actor's audience by the tone -- a string.
 */
int
audience_filter_tone (lua_State *L, struct Actor *actor, const char *tone);

/*
 * Set the entire dialogue tree as the audience.
 */
void
tone_yell (lua_State *L, struct Actor *actor);

/*
 * Set just the children of the actor as the audience.
 */
void
tone_command (lua_State *L, struct Actor *actor);

/*
 * Set an audience is the actor's parent and the children of that parent.
 */
void
tone_say (lua_State *L, struct Actor *actor);

/*
 * Set a specific actor as the audience.
 */
void
tone_whisper (lua_State *L, struct Actor *actor);

/*
 * Set itself as its audience.
 */
void
tone_think (lua_State *L, struct Actor *actor);

#endif
