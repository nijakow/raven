#ifndef RAVEN_LANG_VARS_H
#define RAVEN_LANG_VARS_H

#include "../core/type.h"
#include "../core/objects/symbol.h"

#define VARS_MAX_ENTRIES 64

struct var {
  struct type*   type;
  struct symbol* name;
};

struct vars {
  struct vars* parent;
  unsigned int fill;
  struct var   vars[VARS_MAX_ENTRIES];
};

void vars_create(struct vars* vars);
void vars_destroy(struct vars* vars);

void vars_mark(struct gc* gc, struct vars* vars);

unsigned int vars_count(struct vars* vars);

void vars_reparent(struct vars* vars, struct vars* parent);

void vars_add(struct vars* vars, struct type* type, struct symbol* name);

bool vars_find(struct vars*   vars,
               struct symbol* name,
               struct type**  type_loc,
               unsigned int*  index_loc);

#endif
