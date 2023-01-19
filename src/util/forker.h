/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_FORKER_H
#define RAVEN_UTIL_FORKER_H

#include "../defs.h"

#include "charpp.h"

/*
 * Forking is no easy task, and it's even harder to do it right.
 * 
 * This is a simple wrapper around the `fork` and `execve` system calls.
 * 
 * It allows you to easily build a char** array for passing arguments to
 * `execve(...)`, making it easy to fork and execute an external program.
 * 
 * It also does the path lookup for you, so you don't have to worry about
 * whether or not the executable is in the current working directory.
 * 
 * I am not a huge fan of calling external programs from C, but sometimes
 * it's necessary - and if you're going to do it, you might as well do it
 * right.
 */
struct forker {
    struct charpp  args;
    struct charpp  env;
    bool           wait;
};

void forker_create(struct forker* forker, const char* executable);
void forker_destroy(struct forker* forker);

void forker_add_arg(struct forker* forker, const char* arg);
void forker_add_env(struct forker* forker, const char* env);
void forker_add_default_env(struct forker* forker);

void forker_enable_wait(struct forker* forker);

bool forker_exec(struct forker* forker);

#endif
