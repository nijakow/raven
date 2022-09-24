/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"

#include "../../raven/raven.h"
#include "../../util/memory.h"

#include "../blueprint.h"

#include "object.h"


struct obj_info OBJECT_INFO = {
  .type  = OBJ_TYPE_OBJECT,
  .mark  = (mark_func)  object_mark,
  .del   = (del_func)   object_del,
  .stats = (stats_func) base_obj_stats
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

  object = base_obj_new(raven_objects(raven),
                        &OBJECT_INFO,
                        sizeof(struct object) + sizeof(any) * instance_size);

  if (object != NULL) {
    object->blue            = blueprint;
    object->was_initialized = false;
    object->heartbeat_next  = NULL;
    object->heartbeat_prev  = NULL;
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

  if (object->heartbeat_prev != NULL)
    *(object->heartbeat_prev) = object->heartbeat_next;
  if (object->heartbeat_next != NULL)
    object->heartbeat_next->heartbeat_prev = object->heartbeat_prev;

  object_unlink(object);
  for (child = object->children; child != NULL; child = child->sibling)
    object_unlink(child);

  if (object->slots != object->payload)
    memory_free(object->slots);

  base_obj_del(&object->_);
}

bool object_get(struct object* object, struct symbol* symbol, any* result) {
  struct blueprint*  blue;
  struct vars*       vars;
  unsigned int       index;

  blue = object_blueprint(object);
  vars = blueprint_vars(blue);

  if (!vars_find(vars, symbol, NULL, &index))
    return false;

  *result = *object_slot(object, index);

  return true;
}

bool object_put(struct object* object, struct symbol* symbol, any value) {
  struct blueprint*  blue;
  struct vars*       vars;
  unsigned int       index;

  blue = object_blueprint(object);
  vars = blueprint_vars(blue);

  if (!vars_find(vars, symbol, NULL, &index))
    return false;

  *object_slot(object, index) = value;

  return true;
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

void object_link_heartbeat(struct object* object, struct object** list) {
  if (object->heartbeat_prev == NULL) {
    if (*list != NULL)
      (*list)->heartbeat_prev = &object->heartbeat_next;
    object->heartbeat_prev =  list;
    object->heartbeat_next = *list;
    *list                  =  object;
  }
}

static void object_switch_blueprint(struct object* object, struct blueprint* bp_new) {
  /*
   * Resize the object, so that new slots can fit.
   */
  struct blueprint*  bp_tmp;
  struct blueprint*  soulmate;
  struct symbol*     name;
  any*               new_slots;
  any*               old_slots;
  unsigned int       new_size;
  unsigned int       offset;
  unsigned int       i;
  unsigned int       i2;

  new_size  = blueprint_get_instance_size(bp_new);

  old_slots = object_slots(object);
  new_slots = memory_alloc(sizeof(any) * new_size);

  if (new_slots != NULL) {
    /*
     * Initialize every slot properly.
     */
    for (i = 0; i < new_size; i++) {
      new_slots[i] = any_nil();
    }

    for (bp_tmp = bp_new; bp_tmp != NULL; bp_tmp = blueprint_parent(bp_tmp)) {
      offset   = vars_offset(blueprint_vars(bp_tmp));
      soulmate = blueprint_soulmate(object_blueprint(object), bp_tmp);
      if (soulmate != NULL) {
        for (i = 0; i < vars_count1(blueprint_vars(bp_tmp)); i++) {
          name    = vars_name_for_local_index(blueprint_vars(bp_tmp), i);
          if (vars_find(blueprint_vars(soulmate), name, NULL, &i2))
            new_slots[offset + i] = old_slots[i2];
        }
      }
    }

    /*
     * Install the new slots.
     */
    object->slot_count = new_size;
    object->slots      = new_slots;

    /*
     * Release the old slots (if they were allocated).
     */
    if (old_slots != object->payload) {
      memory_free(old_slots);
    }
  }

  /*
   * Do the magic operation!
   */
  object->blue = bp_new;
}

void object_recompile(struct object* object) {
  struct blueprint*  new_bp;

  new_bp = blueprint_recompile(object_blueprint(object));

  if (new_bp != NULL) {
    object_switch_blueprint(object, new_bp);
  }
}
