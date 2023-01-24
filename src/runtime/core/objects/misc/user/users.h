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
    struct raven*  raven;
    struct user*   list;
};

void users_create(struct users* users, struct raven* raven);
void users_destroy(struct users* users);

struct user* users_find(struct users* users, const char* name);
struct user* users_login(struct users* users, const char* name, const char* password);

#endif
