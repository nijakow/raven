/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "type.h"


void type_create(struct type* type, struct type* parent) {
  type->parent = parent;
}

void type_destroy(struct type* type) {
}



void typeset_create(struct typeset* ts) {
  type_create(&ts->any_type, NULL);
  type_create(&ts->int_type, &ts->any_type);
  type_create(&ts->char_type, &ts->any_type);
  type_create(&ts->string_type, &ts->any_type);
  type_create(&ts->object_type, &ts->any_type);
}

void typeset_destroy(struct typeset* ts) {
  type_destroy(&ts->object_type);
  type_destroy(&ts->string_type);
  type_destroy(&ts->char_type);
  type_destroy(&ts->int_type);
  type_destroy(&ts->any_type);
}
