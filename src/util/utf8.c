/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "utf8.h"

/*
 * A quick reminder on how UTF-8 works:
 *
 *     1 byte: 0xxxxxxx
 *     2 bytes: 110xxxxx 10xxxxxx
 *     3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
 *     4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 * 
 * The following are not valid UTF-8 sequences, but are included for
 * completeness (and I have seen them in the wild before):
 * 
 *     5 bytes: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *     6 bytes: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *     7 bytes: 11111110 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */


/*
 * Decodes a UTF-8 sequence into a codepoint.
 *
 * Returns the codepoint, or 0 if the string is empty.
 * 
 * NOTE: I don't like the signature of this function. I think it should
 *       rather look like this:
 * 
 *           bool utf8_decode(const char* str, raven_rune_t* rune, size_t* len);
 * 
 */
raven_rune_t utf8_decode(const char* str, size_t* len) {
    raven_rune_t rune = 0;

    /*
     * The first byte of a UTF-8 sequence determines the length of the
     * sequence. The first byte is also used to determine the number of
     * bits in the codepoint.
     */
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

/*
 * This function encodes a Unicode codepoint into a UTF-8 sequence.
 *
 * The return value is the length of the UTF-8 sequence. If the
 * codepoint is invalid, we return 0.
 * 
 * We assume that the buffer is large enough to hold the UTF-8 sequence
 * (i.e. at least 4 bytes). We do not check for buffer overflows,
 * and we do not null-terminate the string.
 */
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

/*
 * Count the number of characters in a UTF-8 string.
 *
 * This is not the same as the number of bytes in the string, since
 * some characters are encoded using multiple bytes.
 * 
 * This is a very naive implementation.
 */
size_t utf8_string_length(const char* str) {
    size_t  index;
    size_t  count;

    count = 0;
    for (index = 0; str[index] != '\0'; index++) {
        count += (str[index] & 0xc0) != 0x80;
    }
    return count;
}
