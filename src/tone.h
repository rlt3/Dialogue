#ifndef DIALOGUE_TONE
#define DIALOGUE_TONE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

struct Actor;

/*
 * Filter an Actor's audience by the tone -- a string.
 */
void
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
