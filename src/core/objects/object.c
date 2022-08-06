#include "../blueprint.h"

#include "object.h"


struct obj_info OBJECT_INFO = {
  .type = OBJ_TYPE_OBJECT,
  .mark = (mark_func) object_mark,
  .del  = (del_func)  object_del
};


static void object_unlink(struct object* object) {
  struct object** ptr;

  if (object->parent != NULL) {
    ptr = &object->parent->children;
    while (*ptr != NULL) {
      if (*ptr == object)
        *ptr = object->sibling;
      else
        ptr = &((*ptr)->sibling);
    }

    object->parent  = NULL;
    object->sibling = NULL;
  }
}


struct object* object_new(struct raven* raven, struct blueprint* blueprint) {
  struct object*  object;
  unsigned int    index;
  unsigned int    instance_size;

  instance_size = blueprint_get_instance_size(blueprint);

  object = base_obj_new(raven, &OBJECT_INFO,
                        sizeof(struct object) + sizeof(any) * instance_size);

  if (object != NULL) {
    object->blue            = blueprint;
    object->was_initialized = false;
    object->parent          = NULL;
    object->sibling         = NULL;
    object->children        = NULL;
    object->slot_count      = instance_size;
    object->slots           = object->payload;
    for (index = 0; index < instance_size; index++)
      object->slots[index] = any_nil();
  }

  return object;
}

void object_mark(struct gc* gc, struct object* object) {
  struct object*  child;
  unsigned int    i;

  gc_mark_ptr(gc, object->blue);
  for (i = 0; i < object->slot_count; i++)
    gc_mark_any(gc, object->slots[i]);
  for (child = object->children; child != NULL; child = child->sibling)
    gc_mark_ptr(gc, child);
  base_obj_mark(gc, &object->_);
}

void object_del(struct object* object) {
  struct object*  child;

  object_unlink(object);
  for (child = object->children; child != NULL; child = child->sibling)
    object_unlink(child);

  base_obj_del(&object->_);
}

struct object* object_clone(struct raven* raven, struct object* original) {
  unsigned int    index;
  struct object*  object;

  object = object_new(raven, object_blueprint(original));

  if (object != NULL) {
    for (index = 0;
         index < object->slot_count && index < original->slot_count;
         index++) {
      object->slots[index] = original->slots[index];
    }
  }

  return object;
}

void object_move_to(struct object* object, struct object* target) {
  object_unlink(object);
  if (target != NULL) {
    object->parent   = target;
    object->sibling  = target->children;
    target->children = object;
  }
}
