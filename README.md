# Dialogue

[![travis-ci status](https://travis-ci.org/rlt3/Dialogue.svg?branch=master)](https://travis-ci.org/rlt3/Dialogue/builds)

Dialogue is framework for Lua 5.2 that's an intersection of the Actor Model and
an Entity Component System. Dialogue is a framework based strongly in the
communication between objects through messages. 

Dialogue hasn't been released yet but is almost ready. Take a look at 
[Plans for the future?](#future) below.

[Official documentation is here, but not yet complete.](https://bythehilt.com/dialogue)

## What is Dialogue?

Dialogue is a message-driven asynchronous framework. It was designed for a
great amount of composition. Actors, this program's 'object' and 'entity', are
blank containers which have scripts. Scripts are the typical 'component' and
define what messages they can handle and how they handle them. 

But Dialogue got its name from more than a simple actor analogy -- at
Dialogue's core is the ability for each actor to implicitly scope their
messages. That's right!  [Actors reach a different audience by the way they
speak to one another](https://bythehilt.com/dialogue/tone#visualizer).

## Getting started

[Download Lua5.2](https://www.lua.org/download.html) or get the Lua5.2
developer files from your system's package manager.

Once you have Lua on your system, clone this repository and compile the program
by issuing the `make` command. There currently isn't a `make install`. Dialogue
is valid C99 with the only depedencies being POSIX threads and GNU readline.

I have personally compiled this on a OSX 10.10.3, Ubuntu 12.04/14.04. Dialogue
doesn't currently support Windows.

## How do I use this?

Once you have it compiled, Dialogue expects a Lua file I typically call
`stage.lua`.  Look into the pong tutorial in the source or in the
[documentation](https://bythehilt.com/dialogue). While both are incomplete they
still offer more than a few valid examples.

    ./dialogue stage.lua

This will boot up the interpreter and spin up the Dialogue you've created.

## <a name="future"></a>Plans for the future?

I plan to release v0.0 when:

* [I have complete documentation up](https://bythehilt.com/dialogue)
* There is at least one tutorial available

This is all very soon (next week or three).

After that, I am looking into being able to record the messages and replay
whatever it is that got recorded. And perhaps even edit it along the way. I
don't think this is a stretch and seems plausible right now. And it has been
my goal all along.
