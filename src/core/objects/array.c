/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../raven.h"
#include "../../util/memory.h"

#include "array.h"

struct obj_info ARRAY_INFO = {
  .type = OBJ_TYPE_ARRAY,
  .mark = (mark_func) array_mark,
  .del  = (del_func)  array_del
};

struct array* array_new(struct raven* raven, unsigned int size) {
  struct array*  array;
  unsigned int   i;

  array = base_obj_new(raven_objects(raven), &ARRAY_INFO,
                       sizeof(struct array) + size * sizeof(any));

  if (array != NULL) {
    array->elements = memory_alloc(sizeof(any) * size);
    /* TODO: What if elements is NULL? */
    array->fill  = size;
    array->alloc = size;
    for (i = 0; i < size; i++)
      array->elements[i] = any_nil();
  }

  return array;
}

void array_mark(struct gc* gc, struct array* array) {
  struct array*  arr;
  unsigned int   i;

  arr = array;
  for (i = 0; i < array_size(arr); i++)
    gc_mark_any(gc, array_get(arr, i));
  base_obj_mark(gc, &arr->_);
}

void array_del(struct array* array) {
  memory_free(array->elements);
  base_obj_del(&array->_);
}


struct array* array_join(struct raven* raven,
                         struct array* a,
                         struct array* b) {
  unsigned int   size_a;
  unsigned int   size_b;
  unsigned int   index;
  struct array*  array;

  size_a = array_size(a);
  size_b = array_size(b);
  array  = array_new(raven, size_a + size_b);

  if (array != NULL) {
    for (index = 0; index < size_a; index++)
      array_put(array, index, array_get(a, index));
    for (index = 0; index < size_b; index++)
      array_put(array, size_a + index, array_get(b, index));
  }

  return array;
}

static bool array_make_space(struct array* array) {
  unsigned int index;
  unsigned int new_alloc;
  any*         new_elements;

  if (array->alloc == 0)
    new_alloc = 8;
  else
    new_alloc = array->alloc * 2;

  new_elements = memory_alloc(sizeof(any) * new_alloc);

  if (new_elements == NULL)
    return false;

  for (index = 0; index < array->fill; index++) {
    for (index = 0; index < array->fill; index++)
      new_elements[index] = array->elements[index];
    if (array->elements != NULL)
      memory_free(array->elements);
    array->elements = new_elements;
    array->alloc    = new_alloc;
  }

  return true;
}

void array_append(struct array* array, any value) {
  if (array->fill >= array->alloc)
    if (!array_make_space(array))
      return;
  array->elements[array->fill++] = value;
}
