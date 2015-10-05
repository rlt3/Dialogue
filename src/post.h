#ifndef DIALOGUE_POST
#define DIALOGUE_POST

#include "dialogue.h"
#include "envelope.h"

/*
 * Send an envelope based on its tone.
 */
void
post (Envelope envelope);

/* 
 * Send a message to a script.
 */
void
post_script (Script *script, Envelope envelope);

/*
 * Send a message to all of the scripts of an actor.
 */
void
post_actor (Actor *actor, Envelope envelope);

/*
 * Recursively send a message to all the actors in a dialogue.
 */
void
post_dialogue (Actor *actor, Envelope envelope);

#endif
