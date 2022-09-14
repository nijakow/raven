/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"
#include "../../raven.h"

#include "../object_table.h"

#include "symbol.h"

struct obj_info SYMBOL_INFO = {
  .type  = OBJ_TYPE_SYMBOL,
  .mark  = (mark_func)  symbol_mark,
  .del   = (del_func)   symbol_del,
  .stats = (stats_func) base_obj_stats
};


static struct symbol* symbol_new(struct object_table* table, const char* name) {
  size_t          name_len;
  struct symbol*  symbol;

  name_len = strlen(name);
  symbol   = base_obj_new(table,
                          &SYMBOL_INFO,
                          sizeof(struct symbol)
                        + sizeof(char) * (name_len + 1));

  if (symbol != NULL) {
    strncpy(symbol->name, name, name_len + 1);
    if (table->symbols != NULL)
      table->symbols->prev = &symbol->next;
    symbol->next    =  table->symbols;
    symbol->prev    = &table->symbols;
    symbol->builtin =  NULL;
    table->symbols  =  symbol;
  }

  return symbol;
}

void symbol_mark(struct gc* gc, struct symbol* symbol) {
  base_obj_mark(gc, &symbol->_);
}

void symbol_del(struct symbol* symbol) {
  base_obj_del(&symbol->_);
}

struct symbol* symbol_find_in(struct object_table* table, const char* name) {
  struct symbol*  symbol;

  for (symbol = table->symbols;
       symbol != NULL;
       symbol = symbol->next) {
    if (strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }

  return symbol_new(table, name);
}

struct symbol* symbol_gensym(struct object_table* table) {
  /*
   * TODO: This function should not put the symbol into the linked list!
   *       It should also not generate a name that can be looked up.
   */
  return symbol_new(table, "__gensym");
}
