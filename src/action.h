/*============================================================================/
 
    The Actions are the primitives of Dialogue. This means everything in the 
  Dialogue happen through these Actions.

  Actions are just tables with a specific format. All Actions begin with their
  id. Dialogue switches through these ids and compares them to the enum below.
  Each entry in the enum describes the high-level operation and format for that
  Action.

  These Actions are implemented by combining the Company and Actor's public
  functions. The Company's functions usually involve extracting the Actor itself
  from the Tree and various tree operations. The Actor's functions act as
  `instance methods' for those extracted Actors.

/============================================================================*/

#ifndef DIALOGUE_ACTION
#define DIALOGUE_ACTION

enum Actions {
    /* 
     * Send a message to a specific actor.
     * { ACTION_SEND, actor, "message" [arg1 [, ... [, argN]]] } 
     */
    ACTION_SEND,

    /* 
     * Have an actor deliver a message via tone. Implemented using ACTION_SEND.
     * { ACTION_DELIVER, actor, "tone", "message" [arg1 [, ... [, argN]]] }
     */
    ACTION_DELIVER,

    /* 
     * Load an actor with an optional `init` message to be called afterwards.
     * { ACTION_LOAD, actor [, "init" [arg1 [, ... [, argN]]] }
     */
    ACTION_LOAD,

    /* 
     * Remove the actor from the working tree, but don't delete it.
     * { ACTION_BENCH, actor }
     */
    ACTION_BENCH,

    /* 
     * Join a benched actor back into the working tree.
     * { ACTION_JOIN, actor }
     */
    ACTION_JOIN,

    /* 
     * Remove an actor from the working tree permanently.
     * { ACTION_DELETE, actor }
     */
    ACTION_DELETE
};

#endif
