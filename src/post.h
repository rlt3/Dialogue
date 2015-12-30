#ifndef DIALOGUE_POST
#define DIALOGUE_POST

#define POST_LIB "Dialogue.Post"

/*
 * The Post is the processing end of Dialogue.
 *
 * The Post receives Envelopes it routes to an open Mailbox. The Postman 
 * assigned to that Mailbox will get the Envelopes and attempt to do the Action
 * required for that Envelope.
 *
 * The Post is loaded when the first message is sent (and the first message 
 * will always be from the main thread!). By then, it expects you to have set
 * the correct values.
 *
 * These are the defaults:
 *
 * Dialogue.Post.Postman.max = 8 -- max number of Postmen
 * Dialogue.Post.Postman.per = 4 -- number of Actors per Postman
 */

int 
luaopen_Dialogue_Post (lua_State *L);

#endif
