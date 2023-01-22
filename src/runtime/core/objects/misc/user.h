/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_RUNTIME_CORE_OBJECTS_MISC_USER_H
#define RAVEN_RUNTIME_CORE_OBJECTS_MISC_USER_H

#include "../../../../defs.h"
#include "../../../../util/ringbuffer.h"
#include "../../any.h"

#include "../../base_obj.h"


struct user {
    struct base_obj  _;

    char*            name;
    char*            password;
};

struct user* user_new(struct raven* raven, const char* name);
void user_mark(struct gc* gc, struct user* user);
void user_del(struct user* user);

bool user_compare_name(struct user* user, const char* name);
bool user_compare_password(struct user* user, const char* password);

#endif
