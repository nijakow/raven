/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_CHARPP_H
#define RAVEN_UTIL_CHARPP_H

#include "../defs.h"


/*
 * Despite the funny name, this is actually a pretty useful data structure,
 * which allows us to store a list of strings in a contiguous block of memory.
 * 
 * This is especially useful for building char** arrays, which are required
 * by some functions in the C standard library.
 * 
 * It works similar to a stringbuilder, but instead of storing a sequence of
 * characters, it stores a sequence of strings.
 * 
 * Its main usage is to build a char** array for passing arguments to `execve(...)`
 * in `forker.c`.
 */
struct charpp {
    char**  data;
    size_t  alloc;
    size_t  fill;
};

void charpp_create(struct charpp* charpp);
void charpp_destroy(struct charpp* charpp);

bool charpp_put(struct charpp* charpp, size_t index, const char* str);
void charpp_append(struct charpp* charpp, const char* str);

char*const* charpp_get_static(struct charpp* charpp);
const char* charpp_get_static_at(struct charpp* charpp, size_t index);

size_t charpp_get_size(struct charpp* charpp);

#endif
