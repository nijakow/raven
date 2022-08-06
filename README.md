# The Raven MUD Driver

```
            ~~~===~~~ nijakow proudly presents ~~~===~~~

        A
        v      __               ____
        |     /..\  /^v^\      |  _ \ __ ___   _____ _ __
        |    _\  /_            | |_) / _` \ \ / / _ \ '_ \
        O   /      \           |  _ < (_| |\ V /  __/ | | |
        |\\//      \\          |_| \_\__,_| \_/ \___|_| |_|

        A   G a m e   o f   T e x t   a n d   F a n t a s y

>
```

## What is Raven?

_Raven_ is a game engine for [Multi User Dungeons](https://en.wikipedia.org/wiki/MUD).
It was created by [@nijakow](https://github.com/nijakow) as an alternative
to existing MUD drivers such as [LDMUD](https://github.com/ldmud/ldmud),
[DGD](https://github.com/dworkin/dgd) and others.

It was written in pure C (no dependencies, but assuming a POSIX-style environment),
and is considered to be a successor to the _Four_ project.

It is released under the [GNU GPL v2](./LICENSE).

## Quickstart Info

Getting the MUD to run is relatively simple:

```
git clone https://github.com/nijakow/raven.git
cd raven/src
make run
```

This of course assumes that you have set the `$RAVEN_MUDLIB` environment
variable to point to a valid Raven Mudlib.

## What is the current state of the project?

_Raven_ already successfully compiles and runs a sophisticated programming
language similar to LDMUD's _LPC_. It is possible to build rooms with it,
log in with multiple users at the same time, and experience a full text-based
game.

However, things like a persistent database, user accounts and some language
primitives are still missing. They will be added in the future.
