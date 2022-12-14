/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../../raven/raven.h"
#include "../../../util/memory.h"

#include "array.h"

struct obj_info ARRAY_INFO = {
    .type  = OBJ_TYPE_ARRAY,
    .mark  = (mark_func)  array_mark,
    .del   = (del_func)   array_del,
    .stats = (stats_func) base_obj_stats
};

struct array* array_new(struct raven* raven, unsigned int size) {
    struct array*  array;
    unsigned int   i;

    array = base_obj_new(raven_objects(raven), &ARRAY_INFO,
                         sizeof(struct array));

    if (array != NULL) {
        array->elements = memory_alloc(sizeof(any) * size);
        if (array->elements == NULL) {
            array->fill  = 0;
            array->alloc = 0;
        } else {
            array->fill  = size;
            array->alloc = size;
            for (i = 0; i < size; i++)
                array->elements[i] = any_nil();
        }
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

    for (index = 0; index < array->fill; index++)
        new_elements[index] = array->elements[index];
    for (index = array->fill; index < new_alloc; index++)
        new_elements[index] = any_nil();
    if (array->elements != NULL)
        memory_free(array->elements);
    array->elements = new_elements;
    array->alloc    = new_alloc;

    return true;
}

void array_append(struct array* array, any value) {
    if (array->fill >= array->alloc)
        if (!array_make_space(array))
            return;
    array->elements[array->fill++] = value;
}

void array_insert(struct array* array, unsigned int index, any value) {
    unsigned int i;

    if (index >= array->fill) {
        array_append(array, value);
        return;
    }

    if (array->fill >= array->alloc)
        if (!array_make_space(array))
            return;
    for (i = array->fill; i > index; i--)
        array->elements[i] = array->elements[i - 1];
    array->elements[index] = value;
    array->fill++;
}

void array_remove(struct array* array, unsigned int index) {
    unsigned int i;

    if (index >= array->fill)
        return;

    for (i = index; i < array->fill - 1; i++)
        array->elements[i] = array->elements[i + 1];
    array->fill--;
}
