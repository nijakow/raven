/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../gc/gc.h"
#include "../util/memory.h"

#include "base_obj.h"

void* base_obj_new(struct object_table* table,
                   struct obj_info*     info,
                   size_t               size) {
  struct base_obj* obj;

  obj = memory_alloc(size);

  if (obj != NULL) {
    obj->info      = info;
    obj->next      = table->objects;
    base_obj_set_forward(obj, NULL);
    base_obj_set_gc_tag(obj, GC_TAG_WHITE);
    table->objects = obj;
  }

  return obj;
}

void base_obj_mark(struct gc* gc, struct base_obj* obj) {
}

void base_obj_dispatch_mark(struct gc* gc, struct base_obj* obj) {
  if (!base_obj_is_marked(obj)) {
    base_obj_mark_gray(obj);
    base_obj_set_forward(obj, gc->mark_list);
    gc->mark_list = obj;
  }
}

void base_obj_mark_children(struct gc* gc, struct base_obj* obj) {
  obj->info->mark(gc, obj);
}

void base_obj_del(void* ptr) {
  memory_free(ptr);
}

void base_obj_stats(void*, struct obj_stats* stats) {
  /* TODO */
}

void base_obj_dispatch_del(void* ptr) {
  if (ptr == NULL)
    return;
  ((struct base_obj*) ptr)->info->del(ptr);
}

void base_obj_dispatch_stats(void* ptr, struct obj_stats* stats) {
  if (ptr == NULL)
    return;
  ((struct base_obj*) ptr)->info->stats(ptr, stats);
}
