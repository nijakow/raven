/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "objects/symbol.h"

#include "object_table.h"

void object_table_create(struct object_table* table, struct raven* raven) {
  table->raven      = raven;
  table->objects    = NULL;
  table->symbols    = NULL;
  table->heartbeats = NULL;
}

void object_table_destroy(struct object_table* table) {
  struct base_obj*  current;

  while (table->objects != NULL) {
    current        = table->objects;
    table->objects = current->next;
    base_obj_dispatch_del(current);
  }
}

void object_table_mark(struct gc* gc, struct object_table* table) {
  struct symbol*  symbol;

  for (symbol = table->symbols; symbol != NULL; symbol = symbol->next)
    gc_mark_ptr(gc, symbol);
}

struct symbol* object_table_find_symbol(struct object_table* table,
                                        const char* name) {
  return symbol_find_in(table, name);
}

struct symbol* object_table_gensym(struct object_table* table) {
  return symbol_gensym(table);
}
