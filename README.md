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
It was created by [nijakow](https://github.com/nijakow) as a lightweight alternative
to existing MUD drivers such as [LDMUD](https://github.com/ldmud/ldmud),
[DGD](https://github.com/dworkin/dgd) and others.

It is written in pure C (no dependencies, but assuming a POSIX-style environment),
and is considered to be a successor to the _Four_ project.

Raven is licensed under the [GNU GPL v2](./LICENSE).

## What is the current state of the project?

_Raven_ already successfully compiles and runs a sophisticated programming
language similar to LDMUD's _LPC_. It is possible to build rooms with it,
log in with multiple users at the same time, and experience a full text-based
game.

However, things like a persistent database are still (actively) being worked on.

## Quickstart Info

Getting the MUD to run is relatively simple:

```
git clone https://github.com/nijakow/raven.git
cd raven/src
make run
```

Alternatively, you can use Docker:

```
docker build -t raven https://github.com/nijakow/raven.git
docker run -e RAVEN_MUDLIB=/var/raven -v /path/to/your/mudlib/src:/var/raven -p 4242:4242 -it raven
```

This of course assumes that you have set the `$RAVEN_MUDLIB` environment
variable to point to a valid Raven Mudlib.

## What is a mudlib and how do I add one?

A typical MUD server acts like a virtual UNIX computer. A mudlib represents
the file system that the server starts out with.

To set up a mudlib for Raven, create this directory structure:

```
  + mudlib/
     |
     +- secure/
         |
         +- master.lpc
         |
         +- base.lpc
```

Then set the variable `$RAVEN_MUDLIB` to point to your `mudlib/` directory. If
you now start the server, it will try to run the `main()` function inside of
`mudlib/secure/master.lpc`.

In order to set up a minimal server, configure your files like this:

```c
/* mudlib/secure/master.lpc */

void main() {
  /*
   * This function is called on startup.
   */
  $print("The server is now running!\n");
  $open_port(4242);
}

```

```c
/* mudlib/secure/base.lpc */

/*
 * This is the file that every object automatically inherits from.
 */

inherit;   /* This statement is important! */

void create() {
  /*
   * The default constructor. Can be left empty.
   */
}

```
