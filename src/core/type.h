/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_TYPE_H
#define RAVEN_CORE_TYPE_H

#include "../defs.h"


struct type {
  struct type* parent;
};

void type_create(struct type* type, struct type* parent);
void type_destroy(struct type* type);


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
