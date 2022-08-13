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

/*
 * Fetch the next bytecode in a fiber.
 */
static inline t_bc next_bc(struct fiber* fiber) {
  return function_bc_at(fiber_top(fiber)->function, fiber_top(fiber)->ip++);
}

/*
 * Fetch the next wordcode in a fiber.
 */
static inline t_wc next_wc(struct fiber* fiber) {
  t_wc  wc;

  wc = function_wc_at(fiber_top(fiber)->function, fiber_top(fiber)->ip);
  fiber_top(fiber)->ip += sizeof(t_wc);
  return wc;
}

/*
 * Fetch the next constant in a fiber.
 */
static inline any next_constant(struct fiber* fiber) {
  return function_const_at(fiber_top(fiber)->function, next_wc(fiber));
}

/*
 * Fetch the next type object in a fiber.
 */
static inline struct type* next_type(struct fiber* fiber) {
  return function_type_at(fiber_top(fiber)->function, next_wc(fiber));
}


/*
 * Run a builtin in a fiber.
 *
 * This function receives the fiber to operate on,
 * the symbol describing the builtin, and the number
 * of arguments on the stack.
 */
void fiber_builtin(struct fiber*  fiber,
                   struct symbol* message,
                   unsigned int   args) {
  unsigned int  index;
  builtin_func  blt;

  /*
   * Receive a pointer to the builtin.
   */
  blt = symbol_builtin(message);

  /*
   * In case of error, we crash.
   */
  if (blt == NULL)
    fiber_crash(fiber); /* TODO: Error message */
  else {
    /*
     * We now have the function pointer, and the arguments, which
     * are still stored on the stack. Let's grab them and stuff
     * them into an array.
     */
    any arglist[args + 1];
    for (index = 0; index <= args; index++)
      arglist[index] = fiber_stack_peek(fiber, args)[index];
    fiber_drop(fiber, args + 1);

    /*
     * Energize!
     *   - Jean-Luc Picard
     */
    blt(fiber, &arglist[1], args);
  }
}


/*
 * Send a message to an object.
 *
 * We take the same arguments as `fiber_builtin`.
 * The stack layout looks like this:
 *
 *         +----------------------+
 *  Top -> | argN                 |
 *         | ...                  |
 *         | arg1                 |
 *         | arg0                 |
 *         | receiver             |
 *         +----------------------+
 */
void fiber_send(struct fiber*  fiber,
                struct symbol* message,
                unsigned int   args) {
  any               new_self;
  struct function*  function;

  /*
   * Grab the receiver.
   */
  new_self = *fiber_stack_peek(fiber, args);

  /*
   * Raven does have a proxy system. If we're sending
   * a message to something that doesn't look like
   * a scripted object (anything that's not an instantiated
   * LPC file), we extract the method to call from a
   * different object - its proxy.
   */
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

  /*
   * Extract the function to call.
   */
  function = any_resolve_func(new_self, message);

  /*
   * Call the function. Or, if it wasn't found, a builtin.
   *
   * We only needed the extracted receiver to look up the
   * function, invoking it will automatically establish
   * the stack frame.
   */
  if (function == NULL)
    fiber_builtin(fiber, message, args);
  else
    fiber_push_frame(fiber, function, args);
}

/*
 * Send a message to a receiver, just like `fiber_send(...)`.
 * But this time, we provide a blueprint as another argument,
 * which will be checked for a parent. Then, the message is
 * sent to this specific parent.
 *
 * This function is used when it comes to evaluating
 * expressions such as
 *
 *    ::create();
 *
 * where it delegates the message to the parent blueprint.
 */
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

/*
 * Execute a `raven_op` operation code on a fiber's configuration.
 *
 * This function is just a very basic dispatch loop. Most of the
 * referenced functions can be found in `op.c`.
 */
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

/*
 * Gobble some values from the stack into an array.
 *
 * When an array gets created in an expression like
 *
 *     {
 *       string* greetings = { "hi", "hello", "hey" };
 *     }
 *
 * what's happening is that the strings "hi", "hello"
 * and "hey" get pushed onto the stack, the first array
 * value being the first one to get pushed:
 *
 *        +-----------------+
 *  Top-> | "hey"           |
 *        | "hello"         |
 *        | "hi"            |
 *        +-----------------+
 *
 * This function pops `size` values from the stack, and
 * puts them into an array.
 */
static void fiber_load_array(struct fiber* fiber, unsigned int size) {
  struct array*  array;

  /*
   * Create the array. (Duh!)
   */
  array = array_new(fiber_raven(fiber), size);

  /*
   * We use black magic to drive the loop!
   * Essentially, it's just
   *
   *     ((size--) > 0)
   *
   * but in pretty.
   */
  while (size --> 0) {
    /*
     * No magic here.
     */
    array_put(array, size, fiber_pop(fiber));
  }

  /*
   * Store the array away...
   */
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
    case RAVEN_BYTECODE_TYPECHECK:
      if (!fiber_op_typecheck(fiber, fiber_get_accu(fiber), next_type(fiber)))
        fiber_crash(fiber);
      break;
    case RAVEN_BYTECODE_TYPECAST:
      fiber_set_accu(fiber, fiber_op_typecast(fiber,
                                              fiber_get_accu(fiber),
                                              next_type(fiber)));
      break;
    default:
      fiber_crash(fiber); /* TODO: Error message */
      break;
    }
  }
}
