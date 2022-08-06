/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_FRAME_H
#define RAVEN_VM_FRAME_H

#include "../defs.h"
#include "../core/any.h"

struct frame {
  struct frame*     prev;
  struct function*  function;
  unsigned int      ip;
  any*              locals;
};

void frame_mark(struct gc* gc, struct frame* frame);

static inline any frame_self(struct frame* frame) {
  return frame->locals[0];
}

static inline any* frame_local(struct frame* frame, unsigned int index) {
  return &frame->locals[index + 1];
}

#endif
