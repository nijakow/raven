/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"

#include "../../raven/raven.h"
#include "../../util/stringbuilder.h"
#include "../../util/memory.h"

#include "string.h"

struct obj_info STRING_INFO = {
    .type  = OBJ_TYPE_STRING,
    .mark  = string_mark,
    .del   = string_del,
    .stats = (stats_func) base_obj_stats
};

struct string* string_new(struct raven* raven, const char* contents) {
    size_t          length;
    struct string*  string;

    length = strlen(contents);

    string = base_obj_new(raven_objects(raven),
                          &STRING_INFO,
                          sizeof(struct string) + (length + 1) * sizeof(char));

    if (string != NULL) {
        strncpy(string->contents, contents, (length + 1) * sizeof(char));
        string->length = (unsigned int) length;
    }

    return string;
}

struct string* string_new_from_stringbuilder(struct raven*         raven,
                                             struct stringbuilder* sb) {
    return string_new(raven, stringbuilder_get_const(sb));
}

void string_mark(struct gc* gc, void* string) {
    struct string* str;

    str = string;
    base_obj_mark(gc, &str->_);
}

void string_del(void* string) {
    struct string* str;

    str = string;
    base_obj_del(&str->_);
}


struct string* string_append(struct raven*  raven,
                             struct string* a,
                             struct string* b) {
    struct stringbuilder  sb;
    struct string*        string;

    stringbuilder_create(&sb);
    stringbuilder_append_str(&sb, string_contents(a));
    stringbuilder_append_str(&sb, string_contents(b));
    string = string_new_from_stringbuilder(raven, &sb);
    stringbuilder_destroy(&sb);

    return string;
}

struct string* string_multiply(struct raven*  raven,
                               struct string* string,
                               unsigned int   count) {
    struct stringbuilder  sb;
    struct string*        result;
    unsigned int          i;

    stringbuilder_create(&sb);

    for (i = 0; i < count; i++) {
        stringbuilder_append_str(&sb, string_contents(string));
    }

    result = string_new_from_stringbuilder(raven, &sb);
    stringbuilder_destroy(&sb);

    return result;
}

bool string_eq(struct string* a, struct string* b) {
    return strcmp(string_contents(a), string_contents(b)) == 0;
}

bool string_less(struct string* a, struct string* b) {
    return strcmp(string_contents(a), string_contents(b)) < 0;
}

struct string* string_substr(struct string* string,
                             unsigned int   from,
                             unsigned int   to,
                             struct raven*  raven) {
    const char*           str;
    unsigned int          i;
    struct string*        result;
    struct stringbuilder  sb;

    stringbuilder_create(&sb);
    str = string_contents(string);
    for (i = 0; str[i] != '\0'; i++) {
        if (i >= from && i < to)
            stringbuilder_append_char(&sb, str[i]);
        if (i >= to) break;
    }
    result = string_new_from_stringbuilder(raven, &sb);
    stringbuilder_destroy(&sb);
    return result;
}

unsigned int string_rune_length(struct string* string) {
    return (unsigned int) utf8_string_length(string_contents(string));
}

raven_rune_t string_at_rune(struct string* string, unsigned int index) {
    size_t        byte_index;
    size_t        delta;

    byte_index = 0;
    while (index --> 0) {
        utf8_decode(string_contents(string) + byte_index, &delta);
        if (delta == 0)
            return 0;
        byte_index += delta;
    }
    return utf8_decode(string_contents(string) + byte_index, &delta);
}
