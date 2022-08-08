/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_BASE_OBJ_H
#define RAVEN_OBJECTS_BASE_OBJ_H

#include "../defs.h"
#include "object_table.h"

typedef void (*mark_func)(struct gc*, void*);
typedef void (*del_func)(void*);

struct obj_info {
  enum obj_type type;
  mark_func     mark;
  del_func      del;
};

enum gc_tag {
  GC_TAG_WHITE,
  GC_TAG_GRAY,
  GC_TAG_BLACK
};

struct base_obj {
  struct obj_info* info;
  struct base_obj* next;
  struct base_obj* forward;
  enum gc_tag      gc_tag;
};

void* base_obj_new(struct object_table* table,
                   struct obj_info*     info,
                   size_t               size);
void  base_obj_mark(struct gc* gc, struct base_obj* obj);
void  base_obj_dispatch_mark(struct gc* gc, struct base_obj* obj);
void  base_obj_mark_children(struct gc* gc, struct base_obj* obj);
void  base_obj_del(void*);
void  base_obj_dispatch_del(void*);

static inline bool base_obj_is(struct base_obj* obj, enum obj_type type) {
  return obj->info->type == type;
}

static inline struct base_obj* base_obj_forward(struct base_obj* obj) {
  return obj->forward;
}

static inline bool base_obj_is_marked(struct base_obj* obj) {
  return obj->gc_tag != GC_TAG_WHITE;
}

static inline void base_obj_mark_white(struct base_obj* obj) {
  obj->gc_tag = GC_TAG_WHITE;
}

static inline void base_obj_mark_gray(struct base_obj* obj) {
  obj->gc_tag = GC_TAG_GRAY;
}

static inline void base_obj_mark_black(struct base_obj* obj) {
  obj->gc_tag = GC_TAG_BLACK;
}

#endif
