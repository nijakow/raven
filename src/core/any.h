/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_ANY_H
#define RAVEN_CORE_ANY_H

/*
 * An ANY is a data structure that can hold either integers,
 * floating point numbers or references to objects. It is
 * implemented by a tagged union.
 *
 * Usage:
 * {
 *    any variable;
 *
 *    variable = any_from_int(42);
 *
 *    if (any_is_int(variable)) {
 *        printf("Integer: %d!\n", any_to_int(variable));
 *    } else if (any_is_ptr(variable)) {
 *        printf("Pointer: %p!\n", any_to_ptr(variable));
 *    } else {
 *        printf("Something different!\n");
 *    }
 * }
 */

#include "../defs.h"


/*
 * The different tags, each corresponding to a field
 * in the union type below.
 */
enum any_type {
  ANY_TYPE_NIL,
  ANY_TYPE_PTR,
  ANY_TYPE_INT,
  ANY_TYPE_CHAR
};

/*
 * The union type.
 */
union any_value {
  void* pointer;
  int   integer;
  char  character;
};

/*
 * The tagged union.
 */
typedef struct {
  enum  any_type  type;
  union any_value value;
} any;

/*
 * Check if an `any` hold a specific type.
 */
static inline bool any_is(any a, enum any_type type) {
  return a.type == type;
}

/*
 * Check if an `any` holds `nil`.
 */
static inline bool any_is_nil(any a) {
  return any_is(a, ANY_TYPE_NIL);
}

/*
 * Create an `any` that holds `nil`.
 */
static inline any any_nil() {
  any a;

  a.type = ANY_TYPE_NIL;
  return a;
}

/*
 * Check if an `any` holds a pointer.
 */
static inline bool any_is_ptr(any a) {
  return any_is(a, ANY_TYPE_PTR);
}

/*
 * Create an `any` that holds a pointer.
 */
static inline any any_from_ptr(void* ptr) {
  any a;

  a.type          = ANY_TYPE_PTR;
  a.value.pointer = ptr;

  return a;
}

/*
 * Convert an `any` into a pointer. No type checks.
 */
static inline void* any_to_ptr(any a) {
  return a.value.pointer;
}

/*
 * Check if an `any` holds an integer.
 */
static inline bool any_is_int(any a) {
  return any_is(a, ANY_TYPE_INT);
}

/*
 * Create an `any` that holds an integer.
 */
static inline any any_from_int(int integer) {
  any a;

  a.type          = ANY_TYPE_INT;
  a.value.integer = integer;

  return a;
}

/*
 * Convert an `any` into an integer.
 */
static inline int any_to_int(any a) {
  if (a.type == ANY_TYPE_CHAR)
    return a.value.character;
  return a.value.integer;
}

/*
 * Check if an `any` holds a character.
 */
static inline bool any_is_char(any a) {
  return any_is(a, ANY_TYPE_CHAR);
}

/*
 * Create an `any` that holds a character.
 */
static inline any any_from_char(char ch) {
  any a;

  a.type            = ANY_TYPE_CHAR;
  a.value.character = ch;

  return a;
}

/*
 * Convert an `any` into a character.
 */
static inline char any_to_char(any a) {
  if (a.type == ANY_TYPE_INT)
    return a.value.integer;
  return a.value.character;
}

/*
 * Check if an `any` holds a value that can be considered `true`.
 */
static inline bool any_bool_check(any a) {
  if (any_is_nil(a))
    return false;
  else if (any_is_int(a) && !any_to_int(a))
    return false;
  else if (any_is_char(a) && !any_to_char(a))
    return false;
  return true;
}

/*
 * Check if an `any` equals another.
 */
bool any_eq(any a, any b);

/*
 * Check if an `any` holds a pointer to a specific object type.
 */
bool any_is_obj(any a, enum obj_type type);

/*
 * The `sizeof` operation on an `any`.
 */
unsigned int any_op_sizeof(any a);

/*
 * Extracts the blueprint of the object held by an `any`.
 */
struct blueprint* any_get_blueprint(any a);

/*
 * Extracts a member function of the object held by an `any`.
 */
struct function* any_resolve_func(any a, struct symbol* message);

#endif
