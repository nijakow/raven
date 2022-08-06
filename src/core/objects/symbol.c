#include <string.h>

#include "symbol.h"

struct obj_info SYMBOL_INFO = {
  .type = OBJ_TYPE_SYMBOL,
  .mark = (mark_func) symbol_mark,
  .del  = (del_func)  symbol_del
};


static struct symbol* symbol_new(struct raven* raven, const char* name) {
  size_t          name_len;
  struct symbol*  symbol;

  name_len = strlen(name);
  symbol   = base_obj_new(raven, &SYMBOL_INFO,
                          sizeof(struct symbol)
                        + sizeof(char) * (name_len + 1));

  if (symbol != NULL) {
    strncpy(symbol->name, name, name_len + 1);
    if (raven->symbols != NULL)
      raven->symbols->prev = &symbol->next;
    symbol->next    =  raven->symbols;
    symbol->prev    = &raven->symbols;
    symbol->builtin =  NULL;
    raven->symbols  =  symbol;
  }

  return symbol;
}

void symbol_mark(struct gc* gc, struct symbol* symbol) {
  base_obj_mark(gc, &symbol->_);
}

void symbol_del(struct symbol* symbol) {
  base_obj_del(&symbol->_);
}

struct symbol* symbol_find_in(struct raven* raven, const char* name) {
  struct symbol*  symbol;

  for (symbol = raven->symbols;
       symbol != NULL;
       symbol = symbol->next) {
    if (strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }

  return symbol_new(raven, name);
}

struct symbol* symbol_gensym(struct raven* raven) {
  /*
   * TODO: This function should not put the symbol into the linked list!
   *       It should also not generate a name that can be looked up.
   */
  return symbol_new(raven, "__gensym");
}
