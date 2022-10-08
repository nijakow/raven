/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_STRING_H
#define RAVEN_OBJECTS_STRING_H

#include "../../defs.h"
#include "../base_obj.h"

struct string {
    struct base_obj  _;
    unsigned int     length;
    char             contents[];
};

struct string* string_new(struct raven* raven, const char* contents);
struct string* string_new_from_stringbuilder(struct raven*         raven,
                                             struct stringbuilder* sb);
void           string_mark(struct gc* gc, void* string);
void           string_del(void* string);

struct string* string_append(struct raven*  raven,
                             struct string* a,
                             struct string* b);
struct string* string_multiply(struct raven*  raven,
                               struct string* string,
                               unsigned int   count);

bool string_eq(struct string* a, struct string* b);

struct string* string_substr(struct string* string,
                             unsigned int   from,
                             unsigned int   to,
                             struct raven*  raven);

static inline unsigned int string_length(struct string* string) {
    return string->length;
}

static inline const char* string_contents(struct string* string) {
    return string->contents;
}

static inline char string_at(struct string* string, unsigned int index) {
    if (index >= string_length(string))
        return '\0';
    return string_contents(string)[index];
}

#endif
