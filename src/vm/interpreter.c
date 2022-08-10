/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

#include "../raven.h"
#include "../lang/bytecodes.h"
#include "../core/objects/array.h"
#include "../core/objects/function.h"
#include "../core/objects/funcref.h"
#include "../core/objects/mapping.h"
#include "../core/objects/object.h"
#include "../core/objects/string.h"
#include "../core/objects/symbol.h"
#include "../core/blueprint.h"

#include "fiber.h"
#include "frame.h"
#include "op.h"

#include "interpreter.h"

static inline t_bc next_bc(struct fiber* fiber) {
  return function_bc_at(fiber_top(fiber)->function, fiber_top(fiber)->ip++);
}

static inline t_wc next_wc(struct fiber* fiber) {
  t_wc  wc;

  wc = function_wc_at(fiber_top(fiber)->function, fiber_top(fiber)->ip);
  fiber_top(fiber)->ip += sizeof(t_wc);
  return wc;
}

static inline any next_constant(struct fiber* fiber) {
  return function_const_at(fiber_top(fiber)->function, next_wc(fiber));
}

static inline struct type* next_type(struct fiber* fiber) {
  return function_type_at(fiber_top(fiber)->function, next_wc(fiber));
}


void fiber_builtin(struct fiber*  fiber,
                   struct symbol* message,
                   unsigned int   args) {
  unsigned int  index;
  builtin_func  blt;

  blt = symbol_builtin(message);
  if (blt == NULL)
    fiber_crash(fiber); /* TODO: Error message */
  else {
    any arglist[args + 1];
    for (index = 0; index <= args; index++)
      arglist[index] = fiber_stack_peek(fiber, args)[index];
    fiber_drop(fiber, args + 1);
    blt(fiber, &arglist[1], args);
  }
}


void fiber_send(struct fiber*  fiber,
                struct symbol* message,
                unsigned int   args) {
  any               new_self;
  struct function*  function;

  new_self = *fiber_stack_peek(fiber, args);

  if (any_is_obj(new_self, OBJ_TYPE_OBJECT)) {
    /* Do nothing */
  } else if (any_is_nil(new_self)) {
    new_self = raven_vars(fiber_raven(fiber))->nil_proxy;
  } else if (any_is_obj(new_self, OBJ_TYPE_STRING)) {
    new_self = raven_vars(fiber_raven(fiber))->string_proxy;
  } else if (any_is_obj(new_self, OBJ_TYPE_ARRAY)) {
    new_self = raven_vars(fiber_raven(fiber))->array_proxy;
  } else if (any_is_obj(new_self, OBJ_TYPE_MAPPING)) {
    new_self = raven_vars(fiber_raven(fiber))->mapping_proxy;
  } else if (any_is_obj(new_self, OBJ_TYPE_SYMBOL)) {
    new_self = raven_vars(fiber_raven(fiber))->symbol_proxy;
  }

  function = any_resolve_func(new_self, message);

  if (function == NULL)
    fiber_builtin(fiber, message, args);
  else
    fiber_push_frame(fiber, function, args);
}

void fiber_super_send(struct fiber*      fiber,
                      struct symbol*     message,
                      unsigned int       args,
                      struct blueprint*  func_bp) {
  struct function*  function;

  if (func_bp == NULL || blueprint_parent(func_bp) == NULL)
    fiber_crash(fiber);
  else {
    function = blueprint_lookup(blueprint_parent(func_bp), message);

    if (function == NULL)
      fiber_crash(fiber);
    else
      fiber_push_frame(fiber, function, args);
  }
}

void fiber_op(struct fiber* fiber, enum raven_op op) {
  any  a;
  any  b;

  switch (op) {
  case RAVEN_OP_EQ:
    fiber_set_accu(fiber,
                   any_from_int(any_eq(fiber_pop(fiber),
                                       fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_INEQ:
    fiber_set_accu(fiber,
                   any_from_int(!any_eq(fiber_pop(fiber),
                                        fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_ADD:
    fiber_set_accu(fiber, fiber_op_add(fiber,
                                       fiber_pop(fiber),
                                       fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_SUB:
    fiber_set_accu(fiber, fiber_op_sub(fiber,
                                       fiber_pop(fiber),
                                       fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_MUL:
    fiber_set_accu(fiber, fiber_op_mul(fiber,
                                       fiber_pop(fiber),
                                       fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_DIV:
    fiber_set_accu(fiber, fiber_op_div(fiber,
                                       fiber_pop(fiber),
                                       fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_MOD:
    fiber_set_accu(fiber, fiber_op_mod(fiber,
                                       fiber_pop(fiber),
                                       fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_LESS:
    fiber_set_accu(fiber, any_from_int(fiber_op_less(fiber,
                                                     fiber_pop(fiber),
                                                     fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_LEQ:
    fiber_set_accu(fiber, any_from_int(fiber_op_leq(fiber,
                                                    fiber_pop(fiber),
                                                    fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_GREATER:
    fiber_set_accu(fiber, any_from_int(fiber_op_greater(fiber,
                                                        fiber_pop(fiber),
                                                        fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_GEQ:
    fiber_set_accu(fiber, any_from_int(fiber_op_geq(fiber,
                                                    fiber_pop(fiber),
                                                    fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_NEGATE:
    fiber_set_accu(fiber, fiber_op_negate(fiber, fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_INDEX:
    fiber_set_accu(fiber, fiber_op_index(fiber,
                                         fiber_pop(fiber),
                                         fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_INDEX_ASSIGN:
    b = fiber_pop(fiber);
    a = fiber_pop(fiber);
    fiber_set_accu(fiber, fiber_op_index_assign(fiber,
                                                a,
                                                b,
                                                fiber_get_accu(fiber)));
    break;
  case RAVEN_OP_SIZEOF:
    fiber_set_accu(fiber, any_from_int(any_op_sizeof(fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_NOT:
    fiber_set_accu(fiber, any_from_int(!any_bool_check(fiber_get_accu(fiber))));
    break;
  case RAVEN_OP_NEW:
    fiber_set_accu(fiber, fiber_op_new(fiber, fiber_get_accu(fiber)));
    break;
  default:
    fiber_crash(fiber);
    break;
  }
}

static void fiber_load_array(struct fiber* fiber, unsigned int size) {
  struct array*  array;

  array = array_new(fiber_raven(fiber), size);

  while (size --> 0) {
    array_put(array, size, fiber_pop(fiber));
  }

  fiber_set_accu(fiber, any_from_ptr(array));
}

static void fiber_load_mapping(struct fiber* fiber, unsigned int size) {
  struct mapping*  mapping;
  any              key;
  any              value;

  mapping = mapping_new(fiber_raven(fiber));

  if (size % 2 != 0) fiber_pop(fiber);

  while (size > 0) {
    value = fiber_pop(fiber);
    key   = fiber_pop(fiber);
    mapping_put(mapping, key, value);
    size -= 2;
  }

  fiber_set_accu(fiber, any_from_ptr(mapping));
}

static void fiber_load_funcref(struct fiber* fiber, any name) {
  struct symbol*   symbol;
  struct funcref*  funcref;

  if (!any_is_obj(name, OBJ_TYPE_SYMBOL))
    fiber_crash(fiber);
  else {
    symbol = any_to_ptr(name);
    funcref = funcref_new(fiber_raven(fiber),
                          frame_self(fiber_top(fiber)),
                          symbol);
    fiber_set_accu(fiber, any_from_ptr(funcref));
  }
}

#define RAVEN_DEBUG_MODE 0

void fiber_interpret(struct fiber* fiber) {
  struct symbol*    message;
  struct base_obj*  obj;

  while (fiber->state == FIBER_STATE_RUNNING) {
    if (function_oob(fiber_top(fiber)->function, fiber_top(fiber)->ip)) {
      fiber_pop_frame(fiber);
      continue;
    }

    if (RAVEN_DEBUG_MODE)
      printf("%s ", symbol_name(function_name(fiber_top(fiber)->function)));

    switch ((enum raven_bytecode) next_bc(fiber)) {
    case RAVEN_BYTECODE_NOOP:
      if (RAVEN_DEBUG_MODE) printf("NOOP\n");
      break;
    case RAVEN_BYTECODE_LOAD_SELF:
      if (RAVEN_DEBUG_MODE) printf("LOAD_SELF\n");
      fiber_set_accu(fiber, frame_self(fiber_top(fiber)));
      break;
    case RAVEN_BYTECODE_LOAD_CONST:
    if (RAVEN_DEBUG_MODE) printf("LOAD_CONST\n");
      fiber_set_accu(fiber, next_constant(fiber));
      break;
    case RAVEN_BYTECODE_LOAD_ARRAY:
      if (RAVEN_DEBUG_MODE) printf("LOAD_ARRAY\n");
      fiber_load_array(fiber, (unsigned int) next_wc(fiber));
      break;
    case RAVEN_BYTECODE_LOAD_MAPPING:
      if (RAVEN_DEBUG_MODE) printf("LOAD_MAPPING\n");
      fiber_load_mapping(fiber, (unsigned int) next_wc(fiber));
      break;
    case RAVEN_BYTECODE_LOAD_FUNCREF:
      if (RAVEN_DEBUG_MODE) printf("LOAD_FUNCREF\n");
      fiber_load_funcref(fiber, next_constant(fiber));
      break;
    case RAVEN_BYTECODE_LOAD_LOCAL:
      if (RAVEN_DEBUG_MODE) printf("LOAD_LOCAL\n");
      fiber_set_accu(fiber, *frame_local(fiber_top(fiber), next_wc(fiber)));
      break;
    case RAVEN_BYTECODE_LOAD_MEMBER:
      if (RAVEN_DEBUG_MODE) printf("LOAD_MEMBER\n");
      if (!any_is_ptr(frame_self(fiber_top(fiber))))
        fiber_crash(fiber);
      else {
        obj = any_to_ptr(frame_self(fiber_top(fiber)));
        if (!base_obj_is(obj, OBJ_TYPE_OBJECT))
          fiber_crash(fiber);
        else
          fiber_set_accu(fiber,
                         *object_slot((struct object*) obj, next_wc(fiber)));
      }
      break;
    case RAVEN_BYTECODE_STORE_LOCAL:
      if (RAVEN_DEBUG_MODE) printf("STORE_LOCAL\n");
      *frame_local(fiber_top(fiber), next_wc(fiber)) = fiber_get_accu(fiber);
      break;
    case RAVEN_BYTECODE_STORE_MEMBER:
      if (RAVEN_DEBUG_MODE) printf("STORE_MEMBER\n");
      if (!any_is_ptr(frame_self(fiber_top(fiber))))
        fiber_crash(fiber);
      else {
        obj = any_to_ptr(frame_self(fiber_top(fiber)));
        if (!base_obj_is(obj, OBJ_TYPE_OBJECT))
          fiber_crash(fiber);
        else
          *object_slot((struct object*) obj, next_wc(fiber)) =
                                                      fiber_get_accu(fiber);
      }
      break;
    case RAVEN_BYTECODE_PUSH_SELF:
      if (RAVEN_DEBUG_MODE) printf("PUSH_SELF\n");
      fiber_push(fiber, frame_self(fiber_top(fiber)));
      break;
    case RAVEN_BYTECODE_PUSH:
      if (RAVEN_DEBUG_MODE) printf("PUSH\n");
      fiber_push(fiber, fiber_get_accu(fiber));
      break;
    case RAVEN_BYTECODE_POP:
      if (RAVEN_DEBUG_MODE) printf("POP\n");
      fiber_set_accu(fiber, fiber_pop(fiber));
      break;
    case RAVEN_BYTECODE_OP:
      if (RAVEN_DEBUG_MODE) printf("OP\n");
      fiber_op(fiber, (enum raven_op) next_wc(fiber));
      break;
    case RAVEN_BYTECODE_SEND:
      message = any_to_ptr(next_constant(fiber));
      if (RAVEN_DEBUG_MODE) printf("SEND %s\n", symbol_name(message));
      fiber_send(fiber, message, (unsigned int) next_wc(fiber));
      break;
    case RAVEN_BYTECODE_SUPER_SEND:
      if (RAVEN_DEBUG_MODE) printf("SUPER_SEND\n");
      message = any_to_ptr(next_constant(fiber));
      fiber_super_send(fiber, message, (unsigned int) next_wc(fiber),
                       function_blueprint(fiber_top(fiber)->function));
      break;
    case RAVEN_BYTECODE_JUMP:
      if (RAVEN_DEBUG_MODE) printf("JUMP\n");
      fiber_top(fiber)->ip = (unsigned int) next_wc(fiber);
      break;
    case RAVEN_BYTECODE_JUMP_IF:
      if (RAVEN_DEBUG_MODE) printf("JUMP_IF\n");
      if (any_bool_check(fiber_get_accu(fiber))) {
        fiber_top(fiber)->ip = (unsigned int) next_wc(fiber);
      } else {
        next_wc(fiber);
      }
      break;
    case RAVEN_BYTECODE_JUMP_IF_NOT:
      if (RAVEN_DEBUG_MODE) printf("JUMP_IF_NOT\n");
      if (any_bool_check(fiber_get_accu(fiber))) {
        next_wc(fiber);
      } else {
        fiber_top(fiber)->ip = (unsigned int) next_wc(fiber);
      }
      break;
    case RAVEN_BYTECODE_RETURN:
      if (RAVEN_DEBUG_MODE) printf("RETURN\n");
      fiber_pop_frame(fiber);
      break;
    case RAVEN_BYTECODE_CAST:
      fiber_set_accu(fiber, fiber_op_cast(fiber,
                                          fiber_get_accu(fiber),
                                          next_type(fiber)));
      break;
    default:
      fiber_crash(fiber); /* TODO: Error message */
      break;
    }
  }
}
