/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_TIME_H
#define RAVEN_UTIL_TIME_H

#include "../defs.h"

raven_timestamp_t raven_now();

bool raven_timestamp_less(raven_timestamp_t a, raven_timestamp_t b);

#endif
