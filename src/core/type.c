/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "type.h"


void type_create(struct type* type) {
  type->parent = NULL;
}

void type_destroy(struct type* type) {
}



void typeset_create(struct typeset* ts) {
  type_create(&ts->any_type);
}

void typeset_destroy(struct typeset* ts) {
  type_destroy(&ts->any_type);
}
