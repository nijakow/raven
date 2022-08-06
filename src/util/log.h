/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_LOG_H
#define RAVEN_UTIL_LOG_H

#include "../defs.h"

struct log {
};

void log_create(struct log* log);
void log_destroy(struct log* log);

void log_printf(struct log* log, const char* format, ...);

#endif
