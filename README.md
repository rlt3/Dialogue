# construction_man.gif

Dialogue isn't ready yet, but will be version 0.0 very, very soon. This readme
should give some insight on what I'm doing, but most likely won't work if you
clone this repo.

# Dialogue

Dialogue is an Actor system for Lua. 

Dialogue is different from other Actor systems because it provides scope for
its messages. An Actor can yell or whisper or just say the message to its
audience. Depending on how it sends its message determines who from the
audience receives that message.

A Dialogue is simply a tree of Actors. An Actor serves as a container for
Scripts. Personified, the Actor receives a message and responds according to
its Scripts. The Script could tell the Actor to yell or say a message. Or it
could tell the Actor to move 15 paces to the left.

The Scripts are what you write.

## Dialogue Tones

Every Actor has an audience. This audience can be filtered by the Actor's tone
to change who in the audience receives a message. Here are the Tones:

#### Yell

No filter is applied. Sends the message to the entire Dialogue tree.

#### Say

Send a message to its parent and the children of that parent.

#### Whisper

Send a message to a specific Actor, a one-to-one communication.

#### Command

Send a message to its children.

#### Think

Send a message internally, to its Scripts.

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
automation and computers to write, not humans. The form is rather simple: { {},
{} }, where the first table is the Scripts of an Actor and the second is its
children.
