/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

#include "../raven/raven.h"
#include "../lang/bytecodes.h"
#include "../core/objects/array.h"
#include "../core/objects/function.h"
#include "../core/objects/funcref.h"
#include "../core/objects/mapping.h"
#include "../core/objects/object.h"
#include "../core/objects/string.h"
#include "../core/objects/symbol.h"
#include "../core/blueprint.h"
#include "../util/log.h"

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
    fiber_crash_msg(fiber, "Builtin was not found!");
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
    fiber_crash_msg(fiber, "Unable to super-send message - parent not found!");
  else {
    function = blueprint_lookup(blueprint_parent(func_bp), message);

    if (function == NULL)
      fiber_crash_msg(fiber, "Unable to super-send message - func not found!");
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
    fiber_crash_msg(fiber, "Undefined operation!");
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


/*
 * Gobble some values from the stack into a mapping.
 *
 * This function works exactly like `fiber_load_array(...)`.
 * The stack layout is just like that of an array - just that
 * both the keys and values get pushed:
 *
 *         +--------------+
 *  Top -> | valueN       |
 *         | keyN         |
 *         | ...          |
 *         | value1       |
 *         | key1         |
 *         | value0       |
 *         | key0         |
 *         +--------------+
 */
static void fiber_load_mapping(struct fiber* fiber, unsigned int size) {
  struct mapping*  mapping;
  any              key;
  any              value;

  /*
   * Create the mapping. (Duh!)
   */
  mapping = mapping_new(fiber_raven(fiber));

  /*
   * Ignore any errors. If somebody decides to go rogue and just
   * push a key without a corresponding value, we boldly ignore it.
   *
   * TODO: We should be nice and issue some kind of error message
   *       in the log. Let's leave that open for the future.
   */
  if (size % 2 != 0) fiber_pop(fiber);

  /*
   * This time we can't use our sorcery that helped us loop
   * inside of `fiber_load_array(...)`, since `size` gets
   * reduced by two on every iteration.
   */
  while (size > 0) {
    value = fiber_pop(fiber);
    key   = fiber_pop(fiber);
    mapping_put(mapping, key, value);
    size -= 2;
  }

  /*
   * There, done!
   */
  fiber_set_accu(fiber, any_from_ptr(mapping));
}

/*
 * Create a reference to a function, given its name.
 *
 * This function performs the actions that correspond to
 * this expression:
 *
 *     &foo
 *
 * The idea is to treat `name` as a symbol (if possible),
 * and then look it up inside of our current `this` object.
 * The resulting function pointer will then be wrapped
 * in a freshly allocated funcref.
 */
static void fiber_load_funcref(struct fiber* fiber, any name) {
  struct symbol*   symbol;
  struct funcref*  funcref;

  /*
   * If we get something other than a symbol, we crash.
   * For obvious reasons, looking up an integer is just silly.
   */
  if (!any_is_obj(name, OBJ_TYPE_SYMBOL))
    fiber_crash_msg(fiber, "Function not found!");
  else {
    /*
     * The rest of this function should be obvious.
     * TODO: We don't check for NULL returns. That's stupid
     *       and dangerous. I'm the one to blame here.
     */
    symbol  = any_to_ptr(name);
    funcref = funcref_new(fiber_raven(fiber),
                          frame_self(fiber_top(fiber)),
                          symbol);
    fiber_set_accu(fiber, any_from_ptr(funcref));
  }
}

/*
 * This is the actual bytecode interpreter loop.
 *
 * At its core, this loop is just a giant switch statement
 * that looks at the next opcode coming in and executes
 * the associated operation.
 *
 * There are a few ways to make this function run faster,
 * such as introducing an actual jump table and using
 * computed gotos. But since speed is not really relevant
 * in a MUD we don't really care about that (for now).
 */
void fiber_interpret(struct fiber* fiber) {
  struct symbol*    message;
  struct base_obj*  obj;
  unsigned int      args;

  /*
   * As long as the fiber wants to keep running...
   *
   * Of course, we should be able to interrupt a running
   * fiber after a while in order to switch to a different
   * process. Some processes might even have an upper limit
   * on how many operations they are allowed to perform.
   * We should keep that in mind for future additions.
   */
  while (fiber->state == FIBER_STATE_RUNNING) {
    /*
     * There are cases where the compiler doesn't issue
     * a return opcode at the end, so if we have reached
     * the end of a function, we treat this as a return
     * opcode and pop our frame. Execution continues
     * with the previous function - or stops completely
     * if there is nothing more to do.
     */
    if (function_oob(fiber_top(fiber)->function, fiber_top(fiber)->ip)) {
      fiber_pop_frame(fiber);
      continue;
    }

    /*
     * Switch on the next bytecode.
     */
    switch ((enum raven_bytecode) next_bc(fiber)) {
    /*
     * The NOOP bytecode just does nothing.
     */
    case RAVEN_BYTECODE_NOOP:
      if (RAVEN_DEBUG_MODE) printf("NOOP\n");
      break;
    /*
     * Load the current `this` object.
     */
    case RAVEN_BYTECODE_LOAD_SELF:
      if (RAVEN_DEBUG_MODE) printf("LOAD_SELF\n");
      fiber_set_accu(fiber, frame_self(fiber_top(fiber)));
      break;
    /*
     * Load a constant from the constant vector.
     */
    case RAVEN_BYTECODE_LOAD_CONST:
      if (RAVEN_DEBUG_MODE) printf("LOAD_CONST\n");
      fiber_set_accu(fiber, next_constant(fiber));
      break;
    /*
     * Load an array from the stack.
     */
    case RAVEN_BYTECODE_LOAD_ARRAY:
      if (RAVEN_DEBUG_MODE) printf("LOAD_ARRAY\n");
      fiber_load_array(fiber, (unsigned int) next_wc(fiber));
      break;
    /*
     * Load a mapping from the stack.
     */
    case RAVEN_BYTECODE_LOAD_MAPPING:
      if (RAVEN_DEBUG_MODE) printf("LOAD_MAPPING\n");
      fiber_load_mapping(fiber, (unsigned int) next_wc(fiber));
      break;
    /*
     * Load a reference to a function.
     */
    case RAVEN_BYTECODE_LOAD_FUNCREF:
      if (RAVEN_DEBUG_MODE) printf("LOAD_FUNCREF\n");
      fiber_load_funcref(fiber, next_constant(fiber));
      break;
    /*
     * Load a local variable.
     */
    case RAVEN_BYTECODE_LOAD_LOCAL:
      if (RAVEN_DEBUG_MODE) printf("LOAD_LOCAL\n");
      fiber_set_accu(fiber, *frame_local(fiber_top(fiber), next_wc(fiber)));
      break;
    /*
     * Load an instance variable.
     */
    case RAVEN_BYTECODE_LOAD_MEMBER:
      if (RAVEN_DEBUG_MODE) printf("LOAD_MEMBER\n");
      if (!any_is_ptr(frame_self(fiber_top(fiber))))
        fiber_crash_msg(fiber, "Unable to lookup member - wrong type!");
      else {
        obj = any_to_ptr(frame_self(fiber_top(fiber)));
        if (!base_obj_is(obj, OBJ_TYPE_OBJECT))
          fiber_crash_msg(fiber, "Unable to lookup member - wrong type!");
        else
          fiber_set_accu(fiber,
                         *object_slot((struct object*) obj, next_wc(fiber)));
      }
      break;
    /*
     * Store a local variable.
     */
    case RAVEN_BYTECODE_STORE_LOCAL:
      if (RAVEN_DEBUG_MODE) printf("STORE_LOCAL\n");
      *frame_local(fiber_top(fiber), next_wc(fiber)) = fiber_get_accu(fiber);
      break;
    /*
     * Store an instance variable.
     */
    case RAVEN_BYTECODE_STORE_MEMBER:
      if (RAVEN_DEBUG_MODE) printf("STORE_MEMBER\n");
      if (!any_is_ptr(frame_self(fiber_top(fiber))))
        fiber_crash_msg(fiber, "Unable to store member - wrong type!");
      else {
        obj = any_to_ptr(frame_self(fiber_top(fiber)));
        if (!base_obj_is(obj, OBJ_TYPE_OBJECT))
          fiber_crash_msg(fiber, "Unable to store member - wrong type!");
        else
          *object_slot((struct object*) obj, next_wc(fiber)) =
                                                      fiber_get_accu(fiber);
      }
      break;
    /*
     * Push `this` to the stack. The accumulator will not be touched.
     */
    case RAVEN_BYTECODE_PUSH_SELF:
      if (RAVEN_DEBUG_MODE) printf("PUSH_SELF\n");
      fiber_push(fiber, frame_self(fiber_top(fiber)));
      break;
    /*
     * Push a constant to the stack. The accumulator will not be touched.
     */
    case RAVEN_BYTECODE_PUSH_CONST:
      if (RAVEN_DEBUG_MODE) printf("PUSH_CONST\n");
      fiber_push(fiber, next_constant(fiber));
      break;
    /*
     * Push the accumulator onto the stack.
     */
    case RAVEN_BYTECODE_PUSH:
      if (RAVEN_DEBUG_MODE) printf("PUSH\n");
      fiber_push(fiber, fiber_get_accu(fiber));
      break;
    /*
     * Pop the top-of-stack into the accumulator.
     */
    case RAVEN_BYTECODE_POP:
      if (RAVEN_DEBUG_MODE) printf("POP\n");
      fiber_set_accu(fiber, fiber_pop(fiber));
      break;
    /*
     * Execute an arithmetic or logical operation.
     */
    case RAVEN_BYTECODE_OP:
      if (RAVEN_DEBUG_MODE) printf("OP\n");
      fiber_op(fiber, (enum raven_op) next_wc(fiber));
      break;
    /*
     * Send a message to an object.
     */
    case RAVEN_BYTECODE_SEND:
      args    = (unsigned int) next_bc(fiber);
      message = any_to_ptr(next_constant(fiber));
      if (RAVEN_DEBUG_MODE) printf("SEND %s\n", symbol_name(message));
      fiber_send(fiber, message, args);
      break;
    /*
     * Invoke a method from our superclass.
     */
    case RAVEN_BYTECODE_SUPER_SEND:
      if (RAVEN_DEBUG_MODE) printf("SUPER_SEND\n");
      args    = (unsigned int) next_bc(fiber);
      message = any_to_ptr(next_constant(fiber));
      fiber_super_send(fiber, message, args,
                       function_blueprint(fiber_top(fiber)->function));
      break;
    /*
     * Jump to a different instruction.
     */
    case RAVEN_BYTECODE_JUMP:
      if (RAVEN_DEBUG_MODE) printf("JUMP\n");
      fiber_top(fiber)->ip = (unsigned int) next_wc(fiber);
      break;
    /*
     * Jump to a different instruction if the accumulator is `true`.
     */
    case RAVEN_BYTECODE_JUMP_IF:
      if (RAVEN_DEBUG_MODE) printf("JUMP_IF\n");
      if (any_bool_check(fiber_get_accu(fiber))) {
        fiber_top(fiber)->ip = (unsigned int) next_wc(fiber);
      } else {
        next_wc(fiber);
      }
      break;
    /*
     * Jump to a different instruction if the accumulator is `false`.
     */
    case RAVEN_BYTECODE_JUMP_IF_NOT:
      if (RAVEN_DEBUG_MODE) printf("JUMP_IF_NOT\n");
      if (any_bool_check(fiber_get_accu(fiber))) {
        next_wc(fiber);
      } else {
        fiber_top(fiber)->ip = (unsigned int) next_wc(fiber);
      }
      break;
    /*
     * Return from the current function.
     */
    case RAVEN_BYTECODE_RETURN:
      if (RAVEN_DEBUG_MODE) printf("RETURN\n");
      fiber_pop_frame(fiber);
      break;
    /*
     * Ensure that the accumulator is of a specific type.
     * If it isn't, crash.
     */
    case RAVEN_BYTECODE_TYPECHECK:
      if (!fiber_op_typecheck(fiber, fiber_get_accu(fiber), next_type(fiber)))
        fiber_crash_msg(fiber, "Typecheck failed!");
      break;
    /*
     * Ensure that the accumulator is of a specific type.
     * If it isn't, convert it. This might fail with a crash.
     */
    case RAVEN_BYTECODE_TYPECAST:
      fiber_set_accu(fiber, fiber_op_typecast(fiber,
                                              fiber_get_accu(fiber),
                                              next_type(fiber)));
      break;
    /*
     * Update the reference to the next catch block.
     */
    case RAVEN_BYTECODE_CATCH:
      frame_set_catch_addr(fiber_top(fiber), next_wc(fiber));
      break;
    /*
     * Invalid opcode. That's of course an error. Boom.
     */
    default:
      fiber_crash_msg(fiber, "Invalid bytecode!");
      break;
    }
  }
}
