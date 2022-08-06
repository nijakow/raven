/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_MEMORY_H
#define RAVEN_UTIL_MEMORY_H

#include "../defs.h"

void* memory_alloc(size_t size);
void* memory_realloc(void* ptr, size_t new_size);
void  memory_free(void* ptr);

#endif
