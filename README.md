# Dialogue

[![travis-ci status](https://travis-ci.org/rlt3/Dialogue.svg?branch=master)](https://travis-ci.org/rlt3/Dialogue/builds)

Dialogue is a framework for Lua5.2. It is an intersection of the 
[Actor Model](https://en.wikipedia.org/wiki/Actor_model) and an 
[Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) 
(ECS) with a way to scope messages so that Actors may talk to one another.

An Actor is the core piece of Dialogue. An Actor is an empty container which
can receive messages. It responds to these messsages via the Scripts which are
given to it. These Scripts are analogous to Components in an ECS.

A Dialogue is simply a tree of Actors. It is the tree which provides the scope
mechanisms for messages. A Script might tell an Actor to 'whisper' a message to
another Actor. Or the Script might tell the Actor to 'yell' the message to the
entire Actor tree. I call these scopes 'Tones'.

Each Actor has its own internal state (where all of its Scripts are contained)
which allows messages to be sent asychronously and potentially in parallel.

The Scripts are what you as a user would define.

## Terminology

I intentially chose a lot of play-like language to hopefully make understanding
this easy and to draw parallels between the real world.

#### Message

The message is information being sent from one Actor to another. In this
framework a message is just a table where the first element is the method of
a Script object. E.g, `{"move", 2, 2}` might move an Actor 2 spaces up and
two spaces right.

#### Tone

In real life, the tone is *how* we say something, but not actually what was 
said. It is the same here: it is how we can scope our Audience regardless of
the message being sent. This is how any given Actor gets it audience:

* Yell - Send the message to the entire Dialogue tree.
* Say - Send a message to its parent and the children of that parent.
* Whisper - Send a message to a specific Actor, a one-to-one communication.
* Command - Send a message to its children.
* Think - Send a message internally, to its Scripts.

#### Audience

The group of Actors which will receive a message filtered by the Tone. An
audience can be one Actor (for a whisper) or the entire Dialogue tree (for a 
yell).

#### Script

Just as an actor in real life reads their lines from a script, Scripts here
tell the Actor how to respond to a message received. That may be sending
another message or by doing something internally. 

In this framework, a Script is a Lua object loaded into an Actor's internal
state.

#### Actor

An actor in a play can take many roles depending on the character they're
playing -- they are fungible. Actors are no different here. They are containers
to be given Scripts which they 'read' from.

The Actor has its own internal state which allows it to be sent messages 
asynchronously.

#### Dialogue

The definition of a dialogue is two or more people conversing with one another.
It's mostly the same here: a Dialogue is a tree of Actors and provides the
ability for messages to be scoped by a Tone.

#### Stage

Most plays take place on a stage. Stage.lua is where I define by Scripts and
Actors in a Dialogue.

##### Other terminology

Some other things aren't necessarily play-like, but are simple enough:

* Envelope - holds the message with all pertinent info on how to send it
* Mailbox - holds a collection of Envelopes to be sent
* Postman - actually does the work of sending the Envelopes from the Mailbox

## Quick start

This is a crash-course into how to make a Dialogue and use it. All the scripts
here are provided in the tutorial folder.

    Dialogue = require 'Dialogue'

Create an Actor that acts as a room. Give it a single Script which is
a dungeon from level 7 with coordinates B, 3.

    room = Dialogue.Actor.new{ {"dungeon", 7, {"B", "3"} }
    
Create the player and monster, give them sprites and weapons

    player = room:child{ {"sprite", "player"}, {"weapon", "axe"} }
    monster = room:child{ {"sprite", "monster"}, {"weapon", "claws"} }

The above is functionally the same as this:

    room = Dialogue.new{
        { {"dungeon", 7, {"B", "3"} },
        {
            {
                { {"sprite", "player"}, {"weapon", "axe"} },
                {}
            },
            {
                { {"sprite", "monster"}, {"weapon", "claws"} },
                {}
            }
        }
    }

It seems verbose to express it in table form, but it is mainly meant for
automation and computers to write, not humans. The form is rather simple: `{ {},
{} }`, where the first table is the Scripts of an Actor and the second is its
children.
