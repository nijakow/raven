/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_OBJECT_TABLE_H
#define RAVEN_CORE_OBJECT_TABLE_H

#include "../defs.h"

struct object_table {
  struct raven*     raven;
  struct base_obj*  objects;
  struct symbol*    symbols;
};

void object_table_create(struct object_table* table, struct raven* raven);
void object_table_destroy(struct object_table* table);

void object_table_mark(struct gc* gc, struct object_table* table);

#endif
