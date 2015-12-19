# Dialogue

[![travis-ci status](https://travis-ci.org/rlt3/Dialogue.svg?branch=master)](https://travis-ci.org/rlt3/Dialogue/builds)

Dialogue is a framework for Lua5.2. It is an intersection of the 
[Actor Model](https://en.wikipedia.org/wiki/Actor_model) and an 
[Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) 
(ECS) with a way to scope messages so that Actors may talk to one another.

## What is Dialogue?

Dialogue is a message-driven asynchronous framework. It was designed so that
many types of the objects (Scripts) could be given to many Actors (blank 
containers). A Dialogue is a tree of Actors which send messages to one another.

Dialogue got its name from more than a simple message analogy -- Dialogue's
primary reasoning is the ability to scope messages. Actors in the tree can be
grouped and logically sectioned off through use of Tones. 

Here are how each Actor sends its message by each Tone:

* Yell - to the entire Dialogue tree.
* Command - to its children.
* Say - to its parent and the children of that parent.
* Whisper - to a specific Actor, a one-to-one communication.
* Think - internally, to its Scripts.

## Crash course

In my opinion, the best way to understand something is to first watch someone
use it, but first there are a couple things to know before moving forward:

* A Script is just a Lua module. The modules only have two requirements: 1) When loaded it returns a table and 2) the table must have a `new` field set.
* Messages are the only way to communicate between Scripts. 

Here's a couple brainstorming exercises. Note that the Scripts don't exist, I'm
just using my head and designing the whole structure before actually creating
anything.

### A simple game

Since actors are just empty containers that you fill with Scripts, here's what
a player might look like in my mind:

    player = room:child{ {"sprite", "player"}, {"weapon", "axe"}, {"controller"} }

The `sprite` is a Script which handles many types and draws them appropriately.
`weapon` lets us have different weapons and `controller` lets us swing them.

    monster = room:child{ {"sprite", "monster"}, {"weapon", "claws"}, {"monster"} }

`monster` is mostly the same, but a little different. The `controller` is the
Script that would handle `input` events from the keyboard (from a different
Actor, of course). Since we need the `monster` Actor to do stuff by itself, we
have to provide some sort of basic AI in the `monster` Script.

What happens when the `player` dispatches the `monster`? Well, we have to 
introduce more challenges, of course.

    room = Dialogue.Actor.new{ {"dungeon", 7, {"B", "3"}}, {"trap", {100, 50}}

The `dungeon` just pulls the drawing data from a set of data. It's on floor 7
with coordinates `(B, 3)`. It also handles the `leave` message so the player can
move to `(B, 4)` or where ever.

For `room` to get a `leave` message, it need to have the `player` and `monster`
as children. This is also how the trap will 'know' when to go off -- when 
Actors move, they send a `moving-to {x, y}` message. The trap listens for an
Actor to move close before springing!

### html parser

Since Actors are just empty containers you fill with Scripts, we can say that
each element in an HTML file is an Actor. 

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

## How do I use this?

#### Requirements

* Lua 5.2
* readline

Right now cloning the repo and using the Makefile is the only method which 
uses 

The Makefile in the repo supports Darwin & Linux. I have compiled this on a OSX
10.10.3, Ubuntu 12.04/14.04.

Compiling with `DIALOGUE_HEADLESS` set to `true` compiles Dialogue as a Lua
module. `DIALOGUE_HEADLESS` set to `false` compiles Dialogue as a program with
the interpreter built-in.

## Plans for the future?

I plan to release v0.0 when:

* Actors can be created arbitrarily while the system is running
* Actors can be remove arbitrarily while the system is running
* There are two ways of creating a Dialogue tree (table vs object methods)
* There is at least one tutorial available
* I have complete documentation up

This is all very, very soon (next week or two) because I've started on all of
these things and the first & third are related.

After that, I am looking into being able to record the messages and replay
whatever it is that got recorded. And perhaps even edit it along the way. I
don't think this is a stretch and seems plausible right now. And it has been
my goal all along.
