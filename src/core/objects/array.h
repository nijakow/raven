/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_OBJECTS_ARRAY_H
#define RAVEN_CORE_OBJECTS_ARRAY_H

#include "../any.h"
#include "../base_obj.h"

struct array {
  struct base_obj  _;
  unsigned int     fill;
  unsigned int     alloc;
  any*             elements;
};

struct array* array_new(struct raven* raven, unsigned int size);
void          array_mark(struct gc* gc, struct array* array);
void          array_del(struct array* array);

struct array* array_join(struct raven* raven,
                         struct array* a,
                         struct array* b);
void array_append(struct array* array, any value);

static inline unsigned int array_size(struct array* array) {
  return array->fill;
}

static inline any array_get(struct array* array, unsigned int i) {
  if (i >= array_size(array))
    return any_nil();
  return array->elements[i];
}

static inline void array_put(struct array* array, unsigned int i, any v) {
  if (i < array_size(array))
    array->elements[i] = v;
}

#endif
