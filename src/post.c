#include <stdlib.h>
#include <pthread.h>
#include "post.h"
#include "actor.h"

void
post_actor (Actor *actor, Envelope *envelope)
{
    Envelope e = *envelope;
    actor_send_envelope(actor, &e);
}

void
post_dialogue (Actor *dialogue, Envelope *envelope)
{
    post_actor(dialogue, envelope);

    if (dialogue->next != NULL)
        post_dialogue(dialogue->next, envelope);

    if (dialogue->child != NULL)
        post_dialogue(dialogue->child, envelope);
}

void
post_tone_think (Envelope *envelope)
{
    post_actor(envelope->author, envelope);
}

void
post_tone_whisper (Envelope *envelope)
{
    post_actor(envelope->recipient, envelope);
}

void
post_tone_yell (Envelope *envelope)
{
    post_dialogue(envelope->author->dialogue, envelope);
}

void
post (Envelope envelope)
{
    //envelope.tone(&envelope);
}
