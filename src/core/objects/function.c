/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"
#include "../../raven.h"

#include "../blueprint.h"
#include "../../lang/bytecodes.h"

#include "function.h"

struct obj_info FUNCTION_INFO = {
  .type = OBJ_TYPE_FUNCTION,
  .mark = (mark_func) function_mark,
  .del  = (del_func)  function_del
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
    function->blueprint      =          NULL;
    function->prev_method    =          NULL;
    function->next_method    =          NULL;
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

/*
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

void function_disassemble(struct function* function) {
  unsigned int  ip;

  while (ip < function_bytecode_count(function)) {
    printf("%4u\n", ip);

    switch (next_bc(function, &ip)) {
    case RAVEN_BYTECODE_NOOP:
      printf("NOOP");
      break;
    case RAVEN_BYTECODE_LOAD_SELF:
      printf("LOAD_SELF");
      break;
    case RAVEN_BYTECODE_LOAD_CONST:
      printf("LOAD_CONST %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_ARRAY:
      printf("LOAD_ARRAY %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_MAPPING:
      printf("LOAD_MAPPING %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_FUNCREF:
      printf("LOAD_FUNCREF %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_LOCAL:
      printf("LOAD_LOCAL %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_LOAD_MEMBER:
      printf("LOAD_MEMBER %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_STORE_LOCAL:
      printf("STORE_LOCAL %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_STORE_MEMBER:
      printf("STORE_MEMBER %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_PUSH_SELF:
      printf("PUSH_SELF");
      break;
    case RAVEN_BYTECODE_PUSH:
      printf("PUSH");
      break;
    case RAVEN_BYTECODE_POP:
      printf("POP");
      break;
    case RAVEN_BYTECODE_OP:
      printf("OP %d", next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_SEND:
      printf("SEND %d %d", next_wc(function, &ip), next_wc(function, &ip));
      break;
    case RAVEN_BYTECODE_SUPER_SEND:
      printf("SUPER_SEND");
      break;
    case RAVEN_BYTECODE_JUMP:
      printf("JUMP");
      break;
    case RAVEN_BYTECODE_JUMP_IF:
      printf("JUMP_IF");
      break;
    case RAVEN_BYTECODE_JUMP_IF_NOT:
      printf("JUMP_IF_NOT");
      break;
    case RAVEN_BYTECODE_RETURN:
      printf("RETURN");
      break;
    default:
      printf("???\n");
      return;
    }

    printf("\n");
  }
}
*/
