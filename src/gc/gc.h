/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_GC_GC_H
#define RAVEN_GC_GC_H

#include "../defs.h"
#include "../core/any.h"
#include "../core/base_obj.h"

struct gc {
  struct raven*     raven;
  struct base_obj*  mark_list;
};

void gc_create(struct gc* gc, struct raven* raven);
void gc_destroy(struct gc* gc);

void gc_mark_ptr(struct gc* gc, void* ptr);
void gc_mark_any(struct gc* gc, any value);

void gc_run(struct gc* gc);

static inline struct raven* gc_raven(struct gc* gc) {
  return gc->raven;
}

#endif
