/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_FUNCTION_H
#define RAVEN_CORE_FUNCTION_H

#include "../../defs.h"
#include "../any.h"
#include "../base_obj.h"

struct function {
  struct base_obj    _;
  struct blueprint*  blueprint;
  struct symbol*     name;
  struct function**  prev_method;
  struct function*   next_method;
  unsigned int       locals;
  bool               varargs;
  unsigned int       bytecode_count;
  t_bc*              bytecodes;
  unsigned int       constant_count;
  any*               constants;
  char               payload[];
};

struct function* function_new(struct raven* raven,
                              unsigned int  locals,
                              bool          varargs,
                              unsigned int  bytecode_count,
                              t_bc*         bytecodes,
                              unsigned int  constant_count,
                              any*          constants);
void function_mark(struct gc* gc, struct function* function);
void function_del(struct function* function);

void function_unlink(struct function* function);
void function_link(struct function* function, struct function** list);

void function_in_blueprint(struct function*  function,
                           struct blueprint* blueprint,
                           struct symbol*    name);


static inline struct symbol* function_name(struct function* func) {
  return func->name;
}

static inline struct blueprint* function_blueprint(struct function* func) {
  return func->blueprint;
}

static inline t_bc function_bc_at(struct function* func, unsigned int index) {
  return func->bytecodes[index];
}

static inline t_wc function_wc_at(struct function* func, unsigned int index) {
  return *((t_wc*) &func->bytecodes[index]);
}

static inline any function_const_at(struct function* func, unsigned int index) {
  return func->constants[index];
}

static inline unsigned int function_bytecode_count(struct function* func) {
  return func->bytecode_count;
}

static inline unsigned int function_local_count(struct function* func) {
  return func->locals;
}

static inline bool function_has_varargs(struct function* func) {
  return func->varargs;
}

static inline bool function_oob(struct function* func, unsigned int index) {
  return index >= func->bytecode_count;
}

#endif
