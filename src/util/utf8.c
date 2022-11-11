/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "utf8.h"

raven_rune_t utf8_decode(const char* str, size_t* len) {
    raven_rune_t rune = 0;

    if (str[0] == '\0') {
        rune = 0;
        if (len != NULL) *len = 0;
    } else if ((str[0] & 0xf8) == 0xf0) {
        rune = (str[0] & 0x07);
        rune = (rune << 6) | (str[1] & 0x3f);
        rune = (rune << 6) | (str[2] & 0x3f);
        rune = (rune << 6) | (str[3] & 0x3f);
        if (len != NULL) *len = 4;
    } else if ((str[0] & 0xf0) == 0xe0) {
        rune = (str[0] & 0x0f);
        rune = (rune << 6) | (str[1] & 0x3f);
        rune = (rune << 6) | (str[2] & 0x3f);
        if (len != NULL) *len = 3;
    } else if ((str[0] & 0xe0) == 0xc0) {
        rune = (str[0] & 0x1f);
        rune = (rune << 6) | (str[1] & 0x3f);
        if (len != NULL) *len = 2;
    } else {
        rune = str[0];
        if (len != NULL) *len = 1;
    }

    return rune;
}

size_t utf8_encode(raven_rune_t rune, char* str) {
    if (rune < 0x80) {
        str[0] = rune;
        return 1;
    } else if (rune < 0x800) {
        str[0] = 0xc0 | (rune >> 6);
        str[1] = 0x80 | (rune & 0x3f);
        return 2;
    } else if (rune < 0x10000) {
        str[0] = 0xe0 | (rune >> 12);
        str[1] = 0x80 | ((rune >> 6) & 0x3f);
        str[2] = 0x80 | (rune & 0x3f);
        return 3;
    } else if (rune < 0x110000) {
        str[0] = 0xf0 | (rune >> 18);
        str[1] = 0x80 | ((rune >> 12) & 0x3f);
        str[2] = 0x80 | ((rune >> 6) & 0x3f);
        str[3] = 0x80 | (rune & 0x3f);
        return 4;
    }

    return 0;
}

size_t utf8_string_length(const char* str) {
    size_t  index;
    size_t  count;

    count = 0;
    for (index = 0; str[index] != '\0'; index++) {
        count += (str[index] & 0xc0) != 0x80;
    }
    return count;
}
