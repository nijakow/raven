/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "time.h"

raven_timestamp_t raven_now() {
    return time(NULL);
}

bool raven_timestamp_less(raven_timestamp_t a, raven_timestamp_t b) {
    return a < b;
}
