#include "../raven.h"
#include "gc.h"

static struct base_obj* gc_pop(struct gc* gc) {
  struct base_obj*  obj;

  obj           = gc->mark_list;
  gc->mark_list = obj->forward;

  return obj;
}

static void gc_clear_mark_list(struct gc* gc) {
  while (gc->mark_list != NULL)
    base_obj_mark_white(gc_pop(gc));
}


void gc_create(struct gc* gc, struct raven* raven) {
  gc->raven     = raven;
  gc->mark_list = NULL;
}

void gc_destroy(struct gc* gc) {
  gc_clear_mark_list(gc);
}

void gc_mark_ptr(struct gc* gc, void* ptr) {
  if (ptr != NULL)
    base_obj_dispatch_mark(gc, ptr);
}

void gc_mark_any(struct gc* gc, any value) {
  if (any_is_ptr(value))
    gc_mark_ptr(gc, any_to_ptr(value));
}

void gc_mark_roots(struct gc* gc) {
  raven_mark(gc, gc_raven(gc));
}

void gc_mark_loop(struct gc* gc) {
  struct base_obj*  obj;

  while (gc->mark_list != NULL) {
    obj = gc_pop(gc);
    base_obj_mark_children(gc, obj);
    base_obj_mark_black(obj);
  }
}

void print_object(any obj);

void gc_collect(struct gc* gc) {
  struct base_obj**  ptr;
  struct base_obj*   obj;

  ptr = &gc_raven(gc)->objects;
  while (*ptr != NULL) {
    obj = *ptr;
    if (base_obj_is_marked(obj)) {
      base_obj_mark_white(obj);
      ptr = &(obj->next);
    } else {
      *ptr = obj->next;
      base_obj_dispatch_del(obj);
    }
  }
}

void gc_run(struct gc* gc) {
  gc_clear_mark_list(gc);
  gc_mark_roots(gc);
  gc_mark_loop(gc);
  gc_collect(gc);
}
