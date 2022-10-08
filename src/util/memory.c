/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

/*
 * The idea behind this file is to hide the system's
 * allocators behind our own functions. This is especially
 * useful since `malloc()`'s specification is quite weird
 * in some cases.
 *
 * For example, allocating zero bytes is allowed by most
 * systems, but the standard explicitly says that such
 * behavior is undefined. The same applies to
 * `realloc`ating NULL.
 */

#include "memory.h"

void* memory_alloc(size_t size) {
    if (size == 0)
        size = 1;
    return malloc(size);
}

void* memory_realloc(void* ptr, size_t new_size) {
    if (ptr == NULL)
        return memory_alloc(new_size);
    return realloc(ptr, new_size);
}

void memory_free(void* ptr) {
    if (ptr != NULL)
        free(ptr);
}


char* memory_strdup(const char* ptr) {
    return strdup(ptr);
}
