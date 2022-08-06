#ifndef RAVEN_OBJECTS_SYMBOL_H
#define RAVEN_OBJECTS_SYMBOL_H

#include "base_obj.h"
#include "../../vm/builtins.h"

struct symbol {
  struct base_obj  _;
  struct symbol*   next;
  struct symbol**  prev;
  builtin_func     builtin;
  char             name[];
};

void symbol_mark(struct gc* gc, struct symbol* symbol);
void symbol_del(struct symbol* symbol);

struct symbol* symbol_find_in(struct raven* raven, const char* name);
struct symbol* symbol_gensym(struct raven* raven);

static inline const char* symbol_name(struct symbol* symbol) {
  return symbol->name;
}

static inline builtin_func symbol_builtin(struct symbol* symbol) {
  return symbol->builtin;
}

static inline void symbol_set_builtin(struct symbol* symbol, builtin_func f) {
  symbol->builtin = f;
}

#endif
