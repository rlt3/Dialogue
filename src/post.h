#ifndef DIALOGUE_POST
#define DIALOGUE_POST

#include "envelope.h"

struct Actor;

void
post_actor (struct Actor *actor, Envelope *envelope);

void
post_dialogue (struct Actor *dialogue, Envelope *envelope);

void
post_tone_think (Envelope *envelope);

void
post_tone_whisper (Envelope *envelope);

void
post_tone_yell (Envelope *envelope);

void
post (Envelope *envelope);

#endif
