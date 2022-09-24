/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_VARS_H
#define RAVEN_LANG_VARS_H

/*
 * `Vars` represents a scoped list of variables.
 *
 * This is useful in many places where variable lookup happens,
 * examples are parts of the compiler and inside of blueprints.
 *
 * It is possible to make an instance of `vars` extend from
 * another, so that different scopes can be represented easily.
 */

#include "../core/type.h"
#include "../core/objects/symbol.h"

#define VARS_MAX_ENTRIES 64

struct var {
  struct type*   type;
  struct symbol* name;
};

struct vars {
  struct vars*  parent;
  unsigned int  fill;
  unsigned int  alloc;
  struct var*   vars;
};

void vars_create(struct vars* vars);
void vars_destroy(struct vars* vars);

void vars_mark(struct gc* gc, struct vars* vars);

static inline unsigned int vars_count1(struct vars* vars) { return vars->fill; }
unsigned int vars_count(struct vars* vars);
unsigned int vars_offset(struct vars* vars);

void vars_reparent(struct vars* vars, struct vars* parent);

void vars_add(struct vars* vars, struct type* type, struct symbol* name);

bool vars_find(struct vars*   vars,
               struct symbol* name,
               struct type**  type_loc,
               unsigned int*  index_loc);

struct symbol* vars_name_for_local_index(struct vars* vars, unsigned int index);

static inline struct vars* vars_parent(struct vars* vars) {
  return vars->parent;
}

#endif
