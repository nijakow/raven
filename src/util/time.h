/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_TIME_H
#define RAVEN_UTIL_TIME_H

#include "../defs.h"

/*
 * Handling time in C is weird and broken. We are paranoid about time_t
 * overflow, so we use a custom type for timestamps.
 * 
 * Of course, this means that we can't be sure that the basic numerical
 * comparison operators will work correctly, so we provide our own.
 */

raven_timestamp_t raven_now();

bool raven_timestamp_less(raven_timestamp_t a, raven_timestamp_t b);

#endif
