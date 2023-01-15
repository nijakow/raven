/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_WRAP_H
#define RAVEN_UTIL_WRAP_H

/*
 * MUDs do a lot of text processing, and one of the most common things
 * to do is to wrap text to a certain margin. This file provides a
 * function for doing that.
 */

#include "stringbuilder.h"

void string_wrap_into(const char*           text,
                      unsigned int          margin,
                      struct stringbuilder* into);

#endif
