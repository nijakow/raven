/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_TYPE_H
#define RAVEN_CORE_TYPE_H

#include "../defs.h"
#include "any.h"

struct type;
struct typeset;

typedef bool (*type_check_func)(struct type*, any value);
typedef bool (*type_cast_func)(struct type*, any* value);

struct type {
  struct typeset*  typeset;
  struct type*     parent;
  type_check_func  check_func;
  type_cast_func   cast_func;
};

void type_create(struct type*     type,
                 struct typeset*  ts,
                 struct type*     parent,
                 type_check_func  check,
                 type_cast_func   cast);
void type_destroy(struct type* type);

bool type_is_any(struct type* type);
bool type_match(struct type* type, struct type* test);
bool type_check(struct type* type, any value);
bool type_cast(struct type* type, any* value);

static inline struct typeset* type_typeset(struct type* type) {
  return type->typeset;
}

static inline struct type* type_parent(struct type* type) {
  return type->parent;
}

static inline type_check_func type_type_check_func(struct type* type) {
  return type->check_func;
}

static inline type_cast_func type_type_cast_func(struct type* type) {
  return type->cast_func;
}


struct typeset {
  struct type  void_type;
  struct type  any_type;

  struct type  int_type;
  struct type  char_type;
  struct type  string_type;
  struct type  object_type;
};

void typeset_create(struct typeset* ts);
void typeset_destroy(struct typeset* ts);

static inline struct type* typeset_type_void(struct typeset* ts) {
  return &ts->void_type;
}

static inline struct type* typeset_type_any(struct typeset* ts) {
  return &ts->any_type;
}

static inline struct type* typeset_type_bool(struct typeset* ts) {
  return &ts->int_type;
}

static inline struct type* typeset_type_int(struct typeset* ts) {
  return &ts->int_type;
}

static inline struct type* typeset_type_char(struct typeset* ts) {
  return &ts->char_type;
}

static inline struct type* typeset_type_string(struct typeset* ts) {
  return &ts->string_type;
}

static inline struct type* typeset_type_object(struct typeset* ts) {
  return &ts->object_type;
}

#endif
