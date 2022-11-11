/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "utf8.h"

raven_rune_t utf8_decode(const char* str, size_t* len);

size_t utf8_encode(raven_rune_t rune, char* str) {
    if (rune < 0x80) {
        str[0] = rune;
        return 1;
    } else if (rune < 0x800) {
        str[0] = 0xC0 | (rune >> 6);
        str[1] = 0x80 | (rune & 0x3F);
        return 2;
    } else if (rune < 0x10000) {
        str[0] = 0xE0 | (rune >> 12);
        str[1] = 0x80 | ((rune >> 6) & 0x3F);
        str[2] = 0x80 | (rune & 0x3F);
        return 3;
    } else if (rune < 0x110000) {
        str[0] = 0xF0 | (rune >> 18);
        str[1] = 0x80 | ((rune >> 12) & 0x3F);
        str[2] = 0x80 | ((rune >> 6) & 0x3F);
        str[3] = 0x80 | (rune & 0x3F);
        return 4;
    }

    return 0;
}
