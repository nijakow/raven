/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_PERSISTENCE_PERSISTENCE_H
#define RAVEN_PERSISTENCE_PERSISTENCE_H

#include "../../defs.h"

struct persistence {
    struct raven*  raven;
};

void persistence_create(struct persistence* persistence, struct raven* raven);
void persistence_destroy(struct persistence* persistence);

#endif
