/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_SCRIPT_H
#define RAVEN_LANG_SCRIPT_H

#include "../defs.h"

struct function* script_compile(struct raven*   raven,
                                const  char*    source,
                                struct mapping* vars,
                                struct log*     log);

#endif
