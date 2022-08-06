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

void type_create(struct type* type);
void type_destroy(struct type* type);


struct typeset {
  struct type  any_type;
};

void typeset_create(struct typeset* ts);
void typeset_destroy(struct typeset* ts);

#endif
