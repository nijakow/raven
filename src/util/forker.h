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

struct forker {
    struct charpp  args;
    struct charpp  env;
    bool           wait;
};

void forker_create(struct forker* forker, const char* executable);
void forker_destroy(struct forker* forker);

void forker_add_arg(struct forker* forker, const char* arg);
void forker_add_env(struct forker* forker, const char* env);

void forker_enable_wait(struct forker* forker);

bool forker_exec(struct forker* forker);

#endif
