#ifndef RAVEN_CORE_ANY_H
#define RAVEN_CORE_ANY_H

#include "../defs.h"

enum any_type {
  ANY_TYPE_NIL,
  ANY_TYPE_PTR,
  ANY_TYPE_INT,
  ANY_TYPE_CHAR
};

union any_value {
  void* pointer;
  int   integer;
  char  character;
};

struct any {
  enum any_type   type;
  union any_value value;
};

typedef struct any any;

static inline bool any_is(any a, enum any_type type) {
  return a.type == type;
}

static inline bool any_is_nil(any a) {
  return any_is(a, ANY_TYPE_NIL);
}

static inline any any_nil() {
  any a;

  a.type = ANY_TYPE_NIL;
  return a;
}

static inline bool any_is_ptr(any a) {
  return any_is(a, ANY_TYPE_PTR);
}

static inline any any_from_ptr(void* ptr) {
  any a;

  a.type          = ANY_TYPE_PTR;
  a.value.pointer = ptr;

  return a;
}

static inline void* any_to_ptr(any a) {
  return a.value.pointer;
}

static inline bool any_is_int(any a) {
  return any_is(a, ANY_TYPE_INT);
}

static inline any any_from_int(int integer) {
  any a;

  a.type          = ANY_TYPE_INT;
  a.value.integer = integer;

  return a;
}

static inline int any_to_int(any a) {
  if (a.type == ANY_TYPE_CHAR)
    return a.value.character;
  return a.value.integer;
}

static inline bool any_is_char(any a) {
  return any_is(a, ANY_TYPE_CHAR);
}

static inline any any_from_char(char ch) {
  any a;

  a.type            = ANY_TYPE_CHAR;
  a.value.character = ch;

  return a;
}

static inline char any_to_char(any a) {
  if (a.type == ANY_TYPE_INT)
    return a.value.integer;
  return a.value.character;
}

static inline bool any_bool_check(any a) {
  if (any_is_nil(a))
    return false;
  else if (any_is_int(a) && !any_to_int(a))
    return false;
  else if (any_is_char(a) && !any_to_char(a))
    return false;
  return true;
}

bool any_eq(any a, any b);
bool any_is_obj(any a, enum obj_type type);

unsigned int any_op_sizeof(any a);

struct blueprint* any_get_blueprint(any a);
struct function* any_resolve_func(any a, struct symbol* message);

#endif
