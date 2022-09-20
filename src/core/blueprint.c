/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../raven/raven.h"
#include "../fs/file.h"
#include "objects/object.h"
#include "objects/function.h"

#include "blueprint.h"

struct obj_info BLUEPRINT_INFO = {
  .type  = OBJ_TYPE_BLUEPRINT,
  .mark  = (mark_func)  blueprint_mark,
  .del   = (del_func)   blueprint_del,
  .stats = (stats_func) base_obj_stats
};

static void blueprint_create(struct blueprint* blue, struct file* file) {
  blue->file    = file;
  blue->parent  = NULL;
  blue->methods = NULL;
  vars_create(&blue->vars);
}

static void blueprint_destroy(struct blueprint* blue) {
  while (blue->methods != NULL)
    function_unlink(blue->methods);
  vars_destroy(&blue->vars);
}

struct blueprint* blueprint_new(struct raven* raven, struct file* file) {
  struct blueprint* blueprint;

  blueprint = base_obj_new(raven_objects(raven),
                           &BLUEPRINT_INFO,
                           sizeof(struct blueprint));

  if (blueprint != NULL) {
    blueprint->raven = raven;
    blueprint_create(blueprint, file);
  }

  return blueprint;
}

void blueprint_del(struct blueprint* blueprint) {
  blueprint_destroy(blueprint);
  base_obj_del(&blueprint->_);
}

void blueprint_mark(struct gc* gc, struct blueprint* blue) {
  struct function*  func;

  if (blue == NULL) return;

  for (func = blue->methods; func != NULL; func = func->next_method)
    gc_mark_ptr(gc, func);

  vars_mark(gc, &blue->vars);
  gc_mark_ptr(gc, blue->parent);

  base_obj_mark(gc, &blue->_);
}

struct object* blueprint_instantiate(struct blueprint* blueprint,
                                     struct raven*     raven) {
  return object_new(raven, blueprint);
}

unsigned int blueprint_get_instance_size(struct blueprint* bp) {
  return vars_count(blueprint_vars(bp));
}

bool blueprint_inherit(struct blueprint* blue, struct blueprint* parent) {
  if (blue->parent != NULL)
    return false;
  else {
    blue->parent = parent;
    vars_reparent(&blue->vars, &parent->vars);
    return true;
  }
}

void blueprint_add_var(struct blueprint* blue,
                       struct type*      type,
                       struct symbol*    name) {
  vars_add(blueprint_vars(blue), type, name);
}

void blueprint_add_func(struct blueprint* blue,
                        struct symbol*    name,
                        struct function*  func) {
  function_in_blueprint(func, blue, name);
}

struct function* blueprint_lookup(struct blueprint* blue, struct symbol* msg) {
  struct function*  function;

  while (blue != NULL) {
    function = blue->methods;

    while (function != NULL) {
      if (function->name == msg)
        return function;
      function = function->next_method;
    }

    blue = blue->parent;
  }

  return NULL;
}

struct blueprint* blueprint_load_relative(struct blueprint* bp,
                                          const  char*      path) {
  struct file*  file;

  file = blueprint_file(bp);
  if (file == NULL) return raven_get_blueprint(blueprint_raven(bp), path);

  file = file_parent(file);
  if (file == NULL) return NULL;

  file = file_resolve_flex(file, path);
  if (file == NULL) return NULL;

  return file_get_blueprint(file);
}
