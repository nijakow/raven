/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_TYPE_H
#define RAVEN_CORE_TYPE_H

#include "../defs.h"

struct typeset;

struct type {
  struct typeset*  typeset;
  struct type*     parent;
};

void type_create(struct type* type, struct typeset* ts, struct type* parent);
void type_destroy(struct type* type);

bool type_is_any(struct type* type);
bool type_match(struct type* type, struct type* test);

static inline struct typeset* type_typeset(struct type* type) {
  return type->typeset;
}

static inline struct type* type_parent(struct type* type) {
  return type->parent;
}


struct typeset {
  struct type  any_type;

  struct type  int_type;
  struct type  char_type;
  struct type  string_type;
  struct type  object_type;
};

void typeset_create(struct typeset* ts);
void typeset_destroy(struct typeset* ts);

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
