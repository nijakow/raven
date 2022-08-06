#ifndef RAVEN_CORE_BLUEPRINT_H
#define RAVEN_CORE_BLUEPRINT_H

#include "../defs.h"
#include "vars.h"
#include "objects/base_obj.h"

struct blueprint {
  struct base_obj    _;
  struct file*       file;
  struct blueprint*  parent;
  struct function*   methods;
  struct vars        vars;
};

struct blueprint* blueprint_new(struct raven* raven, struct file* file);
void blueprint_mark(struct gc* gc, struct blueprint* blue);
void blueprint_del(struct blueprint* blueprint);

struct blueprint* blueprint_load_relative(struct blueprint* bp,
                                          const char* path);

unsigned int blueprint_get_instance_size(struct blueprint* bp);

bool blueprint_inherit(struct blueprint* blue, struct blueprint* parent);

void blueprint_add_var(struct blueprint* blue,
                       struct type*      type,
                       struct symbol*    name);
void blueprint_add_func(struct blueprint* blue,
                        struct symbol*    name,
                        struct function*  func);

struct function* blueprint_lookup(struct blueprint* blue, struct symbol* msg);

static inline struct blueprint* blueprint_parent(struct blueprint* blue) {
  return blue->parent;
}

static inline struct file* blueprint_file(struct blueprint* blue) {
  return blue->file;
}

static inline struct vars* blueprint_vars(struct blueprint* bp) {
  return &bp->vars;
}

#endif
