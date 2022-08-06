/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_OBJECT_H
#define RAVEN_OBJECTS_OBJECT_H

#include "base_obj.h"
#include "../any.h"

struct object {
  struct base_obj    _;
  bool               was_initialized;
  struct object*     parent;
  struct object*     sibling;
  struct object*     children;
  struct blueprint*  blue;
  unsigned int       slot_count;
  any*               slots;
  any                payload[];
};

struct object* object_new(struct raven* raven, struct blueprint* blueprint);
void           object_mark(struct gc* gc, struct object* object);
void           object_del(struct object* object);

struct object* object_clone(struct raven* raven, struct object* original);

void object_move_to(struct object* object, struct object* target);

static inline bool object_was_initialized(struct object* object) {
  return object->was_initialized;
}

static inline void object_set_initialized(struct object* object) {
  object->was_initialized = true;
}

static inline struct object* object_parent(struct object* object) {
  return object->parent;
}

static inline struct object* object_sibling(struct object* object) {
  return object->sibling;
}

static inline struct object* object_children(struct object* object) {
  return object->children;
}

static inline struct blueprint* object_blueprint(struct object* object) {
  return object->blue;
}

static inline any* object_slot(struct object* object, unsigned int index) {
  /* TODO: Check against out-of-bounds */
  return &object->slots[index];
}

#endif
