/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_FUNCREF_H
#define RAVEN_VM_FUNCREF_H

#include "../../defs.h"
#include "../any.h"
#include "../base_obj.h"

struct funcref {
  struct base_obj  _;
  any              receiver;
  struct symbol*   message;
};

struct funcref* funcref_new(struct raven*  raven,
                            any            receiver,
                            struct symbol* msg);
void   funcref_mark(struct gc* gc, void* funcref);
void   funcref_del(void* funcref);

void funcref_enter(struct funcref* funcref,
                   struct fiber*   fiber,
                   any*            args,
                   unsigned int    arg_count);

#endif
