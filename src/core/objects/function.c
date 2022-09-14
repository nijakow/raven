/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"

#include "../../raven/raven.h"
#include "../../lang/bytecodes.h"

#include "../blueprint.h"

#include "function.h"

struct obj_info FUNCTION_INFO = {
  .type  = OBJ_TYPE_FUNCTION,
  .mark  = (mark_func)  function_mark,
  .del   = (del_func)   function_del,
  .stats = (stats_func) base_obj_stats
};


struct function* function_new(struct raven* raven,
                              unsigned int  locals,
                              bool          varargs,
                              unsigned int  bytecode_count,
                              t_bc*         bytecodes,
                              unsigned int  constant_count,
                              any*          constants,
                              unsigned int  type_count,
                              struct type** types) {
  struct function*  function;
  unsigned int      index;

  function = base_obj_new(raven_objects(raven),
                          &FUNCTION_INFO,
                          sizeof(struct function)
                            + sizeof(t_bc) * bytecode_count
                            + sizeof(any) * constant_count
                            + sizeof(struct type*) * type_count);

  if (function != NULL) {
    function->name           =          NULL;
    function->blueprint      =          NULL;
    function->prev_method    =          NULL;
    function->next_method    =          NULL;
    function->modifier       =          RAVEN_MODIFIER_NONE;
    function->locals         =          locals;
    function->varargs        =          varargs;
    function->bytecode_count =          bytecode_count;
    function->bytecodes      = (t_bc*) &function->payload[0];
    function->constant_count =          constant_count;
    function->constants      =
      (void*) &function->payload[bytecode_count * sizeof(t_bc)];
    function->type_count     =          type_count;
    function->types          =
      (void*) &function->payload[bytecode_count * sizeof(t_bc)
                               + constant_count * sizeof(any)];

    for (index = 0; index < bytecode_count; index++)
      function->bytecodes[index] = bytecodes[index];
    for (index = 0; index < constant_count; index++)
      function->constants[index] = constants[index];
    for (index = 0; index < type_count; index++)
      function->types[index] = types[index];
  }

  return function;
}

void function_mark(struct gc* gc, struct function* function) {
  struct function* func;
  unsigned int     i;

  func = function;
  gc_mark_ptr(gc, func->blueprint);
  gc_mark_ptr(gc, func->name);
  for (i = 0; i < func->constant_count; i++)
    gc_mark_any(gc, func->constants[i]);
  base_obj_mark(gc, &func->_);
}

void function_del(struct function* function) {
  function_unlink(function);
  base_obj_del(&function->_);
}

void function_unlink(struct function* function) {
  if (function->blueprint != NULL) {
    if (function->prev_method != NULL)
      *function->prev_method = function->next_method;
    if (function->next_method != NULL)
      function->next_method->prev_method = function->prev_method;
    function->blueprint = NULL;
  }
}

void function_link(struct function* function, struct function** list) {
  if (*list != NULL)
    (*list)->prev_method = &function->next_method;
  function->prev_method =  list;
  function->next_method = *list;
  *list = function;
}


void function_in_blueprint(struct function*  function,
                           struct blueprint* blueprint,
                           struct symbol*    name) {
  if (function->blueprint == NULL) {
    function->blueprint = blueprint;
    function->name      = name;
    function_link(function, &blueprint->methods);
  }
}


static inline t_bc next_bc(struct function* function, unsigned int* ip) {
  *ip += sizeof(t_bc);
  return function_bc_at(function, *ip - sizeof(t_bc));
}

static inline t_wc next_wc(struct function* function, unsigned int* ip) {
  *ip += sizeof(t_wc);
  return function_wc_at(function, *ip - sizeof(t_wc));
}

static inline any next_constant(struct function* func, unsigned int* ip) {
  return function_const_at(func, next_wc(func, ip));
}

void function_disassemble(struct function* function, struct log* log) {
  unsigned int  ip;
  unsigned int  args;

  if (function_name(function) == NULL)
    log_printf(log, "Function <unnamed>:\n");
  else {
    log_printf(log, "Function %s:\n", symbol_name(function_name(function)));
  }

  ip = 0;

  while (ip < function_bytecode_count(function)) {
    log_printf(log, "%4u ", ip);

    switch (next_bc(function, &ip)) {
    case RAVEN_BYTECODE_NOOP:
      log_printf(log, "NOOP");
      break;
    case RAVEN_BYTECODE_LOAD_SELF:
      log_printf(log, "LOAD_SELF");
      break;
    case RAVEN_BYTECODE_LOAD_CONST:
      log_printf(log, "LOAD_CONST %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_ARRAY:
      log_printf(log, "LOAD_ARRAY %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_MAPPING:
      log_printf(log, "LOAD_MAPPING %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_FUNCREF:
      log_printf(log, "LOAD_FUNCREF %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_LOCAL:
      log_printf(log, "LOAD_LOCAL %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_MEMBER:
      log_printf(log, "LOAD_MEMBER %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_STORE_LOCAL:
      log_printf(log, "STORE_LOCAL %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_STORE_MEMBER:
      log_printf(log, "STORE_MEMBER %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_PUSH_SELF:
      log_printf(log, "PUSH_SELF");
      break;
    case RAVEN_BYTECODE_PUSH_CONST:
      log_printf(log, "PUSH_CONST %u", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_PUSH:
      log_printf(log, "PUSH");
      break;
    case RAVEN_BYTECODE_POP:
      log_printf(log, "POP");
      break;
    case RAVEN_BYTECODE_OP:
      log_printf(log, "OP %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_SEND:
      args = (unsigned int) next_bc(function, &ip);
      log_printf(log, "SEND %u %s",
                      args,
                      symbol_name(any_to_ptr(next_constant(function, &ip))));
      break;
    case RAVEN_BYTECODE_SUPER_SEND:
      args = (unsigned int) next_bc(function, &ip);
      log_printf(log, "SUPER_SEND %u %s",
                      args,
                      symbol_name(any_to_ptr(next_constant(function, &ip))));
      break;
    case RAVEN_BYTECODE_JUMP:
      log_printf(log, "JUMP %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_JUMP_IF:
      log_printf(log, "JUMP_IF %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_JUMP_IF_NOT:
      log_printf(log, "JUMP_IF_NOT %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_RETURN:
      log_printf(log, "RETURN");
      break;
    case RAVEN_BYTECODE_TYPECHECK:
      log_printf(log, "TYPECHECK %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_TYPECAST:
      log_printf(log, "TYPECHECK %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_CATCH:
      log_printf(log, "CATCH %d", next_wc(function, &ip));
      break;
    default:
      log_printf(log, "???\n");
      return;
    }

    log_printf(log, "\n");
  }
}
