#include "post.h"
#include <stdio.h>
#include <pthread.h>

/*
 * Send an envelope based on its tone.
 */
void
post (Envelope envelope)
{
    envelope.tone(envelope);
}

/*
 * Send an Envelope to an Actor.
 */
void
post_actor (Actor *actor, Envelope envelope)
{
    envelope_push_table(envelope->L,
}

/*
 * Recursively send a message to all the actors in a dialogue.
 */
void
post_dialogue (Actor *actor, Envelope envelope)
{
    post_actor(actor, envelope);

    if (actor->next != NULL)
        post_dialogue(actor->next, envelope);

    if (actor->child != NULL)
        post_dialogue(actor->child, envelope);
}
