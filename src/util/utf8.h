/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_UTF8_H
#define RAVEN_UTIL_UTF8_H

#include "../defs.h"

/*
 * Yes, Raven does support Unicode and UTF-8. This is a very simple
 * implementation of UTF-8, which is sufficient for our purposes.
 * 
 * We use a 32-bit integer to represent a Unicode codepoint, which is
 * fine since we don't expect the Unicode table to explode anytime soon.
 * 
 * This even is enough for emoji, which is nice, because that means we
 * can have a little fun with our MUD.
 * 
 * We don't support UTF-16 or UTF-32, as they are going to collide with
 * Telnet's IAC character anyway, and we really don't want to deal with
 * that.
 */

typedef uint32_t raven_rune_t;

raven_rune_t utf8_decode(const char* str, size_t* len);
size_t       utf8_encode(raven_rune_t rune, char* str);

size_t       utf8_string_length(const char* str);

#endif
