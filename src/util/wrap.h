/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_WRAP_H
#define RAVEN_UTIL_WRAP_H

#include "stringbuilder.h"

void string_wrap_into(const char*           text,
                      unsigned int          margin,
                      struct stringbuilder* into);

#endif
