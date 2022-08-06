/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "frame.h"

void frame_mark(struct gc* gc, struct frame* frame) {
  gc_mark_ptr(gc, frame->function);
}
