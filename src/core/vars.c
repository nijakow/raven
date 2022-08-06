#include "vars.h"

void vars_create(struct vars* vars) {
  vars->parent = NULL;
  vars->fill   = 0;
}

void vars_destroy(struct vars* vars) {

}

void vars_mark(struct gc* gc, struct vars* vars) {
  unsigned int i;

  if (vars == NULL) return;

  for (i = 0; i < vars->fill; i++) {
    gc_mark_ptr(gc, vars->vars[i].name);
  }

  vars_mark(gc, vars->parent);
}

unsigned int vars_count(struct vars* vars) {
  unsigned int count;

  count = 0;
  while (vars != NULL) {
    count = count + vars->fill;
    vars  = vars->parent;
  }

  return count;
}

void vars_reparent(struct vars* vars, struct vars* parent) {
  vars->parent = parent;
}

void vars_add(struct vars* vars, struct type* type, struct symbol* name) {
  if (vars->fill < VARS_MAX_ENTRIES) {
    vars->vars[vars->fill].type = type;
    vars->vars[vars->fill].name = name;
    vars->fill++;
  }
}

bool vars_find(struct vars*   vars,
               struct symbol* name,
               struct type**  type_loc,
               unsigned int*  index_loc) {
  unsigned int i;

  while (vars != NULL) {
    /*
     * Iterate through the vars and find the correct one
     */
    for (i = 0; i < vars->fill; i++) {
      if (vars->vars[i].name == name) {
        *type_loc  = vars->vars[i].type;
        *index_loc = vars_count(vars->parent) + i;
        return true;
      }
    }
    vars = vars->parent;
  }

  return false;
}
