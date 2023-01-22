/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_RUNTIME_CORE_OBJECTS_MISC_USER_USERS_H
#define RAVEN_RUNTIME_CORE_OBJECTS_MISC_USER_USERS_H

#include "../../../../../defs.h"

struct users {
    struct user*  list;
};

void users_create(struct users* users);
void users_destroy(struct users* users);

#endif
