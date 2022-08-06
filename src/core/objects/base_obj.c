#include "../../gc/gc.h"
#include "../../util/memory.h"

#include "base_obj.h"

void* base_obj_new(struct raven* raven, struct obj_info* info, size_t size) {
  struct base_obj* obj;

  obj = memory_alloc(size);

  if (obj != NULL) {
    obj->info      = info;
    obj->next      = raven->objects;
    obj->forward   = NULL;
    obj->gc_tag    = GC_TAG_WHITE;
    raven->objects = obj;
  }

  return obj;
}

void base_obj_mark(struct gc* gc, struct base_obj* obj) {
}

void base_obj_dispatch_mark(struct gc* gc, struct base_obj* obj) {
  if (!base_obj_is_marked(obj)) {
    base_obj_mark_gray(obj);
    obj->forward  = gc->mark_list;
    gc->mark_list = obj;
  }
}

void base_obj_mark_children(struct gc* gc, struct base_obj* obj) {
  if (obj->forward != obj)
    obj->info->mark(gc, obj);
}

void base_obj_del(void* ptr) {
  memory_free(ptr);
}

void base_obj_dispatch_del(void* ptr) {
  if (ptr == NULL)
    return;
  ((struct base_obj*) ptr)->info->del(ptr);
}
