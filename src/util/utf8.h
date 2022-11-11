/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_UTF8_H
#define RAVEN_UTIL_UTF8_H

#include "../defs.h"

typedef uint32_t raven_rune_t;

raven_rune_t utf8_decode(const char* str, size_t* len);
size_t       utf8_encode(raven_rune_t rune, char* str);

size_t       utf8_string_length(const char* str);

#endif
