# Dialogue

[![travis-ci status](https://travis-ci.org/rlt3/Dialogue.svg?branch=master)](https://travis-ci.org/rlt3/Dialogue/builds)

Dialogue is a framework for Lua5.2. It is an intersection of the 
[Actor Model](https://en.wikipedia.org/wiki/Actor_model) and an 
[Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) 
(ECS) with a way to scope messages so that Actors may talk to one another.

Dialogue hasn't been released yet but is almost ready. Take a look at 
[Plans for the future?](#future) below.

## What is Dialogue?

Dialogue is a message-driven asynchronous framework. It was designed so that
many types of objects (Scripts) could be given to many Actors (blank
containers). A Dialogue is a tree of Actors which send messages to one another.

But Dialogue got its name from more than a simple message analogy -- Dialogue's
primary reasoning is the ability to scope messages. Actors in the tree can be
grouped and logically sectioned off through use of Tones which scope each
message to a specific Audience of an actor.

Here is the Audience for each Actor via the Tone.

* Yell - the entire Dialogue tree.
* Command - its children.
* Say - its parent and the children of that parent.
* Whisper - a specific Actor, a one-to-one communication.
* Think - itself, each of its Scripts

## Crash course

In my opinion, the best way to understand something is to first watch someone
use it, but there are a couple things to know before moving forward:

* A Script is just a Lua module. The modules only have two requirements: 1) When loaded it returns a table and 2) the table must have a `new` field set.
* Messages are the only way to communicate between Scripts. And, thus, between Actors

Here's a couple brainstorming exercises. Note that these Scripts don't exist,
I'm just using my head and designing the whole structure before actually
creating anything.

### A simple game

Since actors are just empty containers that you fill with Scripts, here's what
a player might look like in my mind:

    player = room:child{ {"sprite", "player"}, {"weapon", "axe"}, {"controller"} }

The `sprite` is a Script which handles many types of Actors and `draw`s them
appropriately.  The `controller` is the Script that would handle events from
the keyboard and send `input` messages to the other Scripts.  In other words,
`weapon` lets us have different weapons and `controller` lets us use them. 

    monster = room:child{ {"sprite", "monster"}, {"weapon", "claws"}, {"monster"} }

`monster` is mostly the same. Since we need the `monster` Actor to do stuff by
itself, we have to provide some sort of basic AI in the `monster` Script.

But what happens when the `player` dispatches the `monster`? Well, we have to 
introduce more challenges and places to go, of course.

    room = Dialogue.Actor.new{ {"dungeon", 7, {"B", "3"}} }
    trap = Dialogue.Actor.new{ {"sprite", "chest"}, {"trap", {100, 50}} }

The `dungeon` is on floor 7 with coordinates (B, 3). It uses those coordinates
to `draw` the right room. It also handles the `leave` message so the player can
move to (B, 4) or where ever.

For `room` to get a `leave` message, it needs to have the `player` and `monster`
as children. `trap` also wants to be a child of `room` to hear when the `player` 
gets close. The `room` and all of its children are in the `say` scope of the
Actors `player`, `room`, and `trap`.

The `player` might `say` (the Tone) a message `moving-to {x, y}` to all
Actors in the `room` and the `room` itself. If it gets too close to `{100, 50}`
an `explode` message might get sent. Or if the `player` finds the `door` (an
Actor we haven't thought of yet) the `leave` message may be sent instead.

### html parser

Since Actors are just empty containers you fill with Scripts, we can say that
each element in an HTML file is an Actor and the children of each element are
children of that Actor.

Let's take the most basic example: `<div id="container"></div>`. Instead of
creating an Actor like: `{ {"element", "div"}, {"id", "container"} }`, we can
just a separate Script for each element.  This is because each element is
a separate unit as far as standards go.

Because Scripts are just Lua modules, there's no reason one couldn't still make
an `attributes` module to use inside the `p`, `div`, `span` Scripts. It would 
be a shame to repeat every attribute for each element. 

Since I don't have tables with keys working for Dialogue yet, table with a set
definition: `{optional id, {optional, class, names} }`. Otherwise, we'd just be
able to look and see if the key exists or not.

Here's some HTML:

    <div id="main-content" class="container">
        <h1> 
            Dialogue
            <small class="subtitle middle"> - something pretty cool</small>
        </h1>
    </div>

And my Dialogue structure after parsing:

    Actor.new{ {"div", {"main-content", {"container"}}} }
            .child{ { "h1", {nil, {}}}, {"text", "Dialogue"} }
                .child{ {"small", {nil, {"subtitle", "middle"}}, {"text", "- something pretty cool"}} }

## Requirements

* Lua 5.2
* readline

The Makefile in the repo supports Darwin & Linux. I have compiled this on a OSX
10.10.3, Ubuntu 12.04/14.04.

Compiling with `DIALOGUE_HEADLESS` set to `true` compiles Dialogue as a Lua
module. `DIALOGUE_HEADLESS` set to `false` compiles Dialogue as a program with
the interpreter built-in.

## How do I use this?

Once you have it compile, Dialogue expects a lua file I typically call 
`stage.lua`. I call it the stage because that's where I define my Dialogue. 
Look at the two "crash-courses" above or into `spec/stage.lua` for a quick and
dirty example of how that's done.

    ./dialogue stage.lua

This will boot up and interpreter and spin up the Dialogue you've created.

## <a name="future"></a>Plans for the future?

I plan to release v0.0 when:

* Actors can be created arbitrarily while the system is running
* Actors can be removed arbitrarily while the system is running
* There are two ways of creating a Dialogue tree (table vs object methods)
* There is at least one tutorial available
* I have complete documentation up

This is all very, very soon (next week or two) because I've started on all of
these things and the first & third are related.

After that, I am looking into being able to record the messages and replay
whatever it is that got recorded. And perhaps even edit it along the way. I
don't think this is a stretch and seems plausible right now. And it has been
my goal all along.
