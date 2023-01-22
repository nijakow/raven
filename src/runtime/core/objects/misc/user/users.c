/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "user.h"

#include "users.h"

void users_create(struct users* users) {
    users->list = NULL;
}

void users_destroy(struct users* users) {
    assert(users->list == NULL);
}
