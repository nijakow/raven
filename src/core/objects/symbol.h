/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_SYMBOL_H
#define RAVEN_OBJECTS_SYMBOL_H

#include "../../defs.h"
#include "../../vm/builtins.h"
#include "../base_obj.h"

struct symbol {
  struct base_obj  _;
  struct symbol*   next;
  struct symbol**  prev;
  builtin_func     builtin;
  char             name[];
};

void symbol_mark(struct gc* gc, struct symbol* symbol);
void symbol_del(struct symbol* symbol);

struct symbol* symbol_find_in(struct object_table* table, const char* name);
struct symbol* symbol_gensym(struct object_table* table);

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
