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
};

struct user* user_new(struct raven* raven);
void user_mark(struct gc* gc, struct user* user);
void user_del(struct user* user);

#endif
