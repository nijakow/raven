/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

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
