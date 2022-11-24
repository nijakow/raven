/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_FRAME_H
#define RAVEN_VM_FRAME_H

#include "../../defs.h"

#include "../core/any.h"
#include "../core/objects/array.h"

struct frame {
    struct frame*     prev;
    struct function*  function;
    unsigned int      catch_addr;
    unsigned int      ip;
    any*              locals;
    struct array*     varargs;
};

void frame_mark(struct gc* gc, struct frame* frame);

static inline struct frame* frame_prev(struct frame* frame) {
    return frame->prev;
}

static inline struct function* frame_function(struct frame* frame) {
    return frame->function;
}

static inline any frame_self(struct frame* frame) {
    return frame->locals[0];
}

static inline any* frame_local(struct frame* frame, unsigned int index) {
    return &frame->locals[index + 1];
}

static inline unsigned int frame_catch_addr(struct frame* frame) {
    return frame->catch_addr;
}

static inline void frame_set_catch_addr(struct frame* frame, unsigned int ca) {
    frame->catch_addr = ca;
}

#endif
