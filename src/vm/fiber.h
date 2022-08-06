/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_FIBER_H
#define RAVEN_VM_FIBER_H

#include "../defs.h"
#include "../core/any.h"
#include "scheduler.h"

struct frame;

enum fiber_state {
  FIBER_STATE_RUNNING,
  FIBER_STATE_PAUSED,
  FIBER_STATE_WAITING_FOR_INPUT,
  FIBER_STATE_STOPPED,
  FIBER_STATE_CRASHED
};

struct fiber_vars {
  any  this_player;
};

struct fiber {
  any                accu;
  struct fiber*      next;
  struct fiber**     prev;
  enum fiber_state   state;
  struct scheduler*  scheduler;
  struct connection* connection;
  struct funcref*    input_to;
  struct fiber_vars  vars;
  struct frame*      top;
  char*              sp;
  char               payload[];
};

void fiber_create(struct fiber* fiber, struct scheduler* scheduler);
void fiber_destroy(struct fiber* fiber);

struct fiber* fiber_new(struct scheduler* scheduler);
void          fiber_delete(struct fiber* fiber);

void fiber_mark(struct gc* gc, struct fiber* fiber);

void fiber_push_frame(struct fiber*    fiber,
                      struct function* func,
                      unsigned int     args);
void fiber_pop_frame(struct fiber* fiber);

void fiber_pause(struct fiber* fiber);
void fiber_wait_for_input(struct fiber* fiber);
void fiber_reactivate(struct fiber* fiber);
void fiber_reactivate_with_value(struct fiber* fiber, any value);
void fiber_do_crash(struct fiber* fiber, const char* file, int line);

#define fiber_crash(fiber) fiber_do_crash(fiber, __FILE__, __LINE__)

void fiber_push_input(struct fiber* fiber, struct string* text);

static inline struct raven* fiber_raven(struct fiber* fiber) {
  return scheduler_raven(fiber->scheduler);
}

static inline struct connection* fiber_connection(struct fiber* fiber) {
  return fiber->connection;
}

static inline void fiber_set_connection(struct fiber* fiber,
                                        struct connection* connection) {
  fiber->connection = connection;
}

static inline void fiber_set_inputto(struct fiber* fiber,
                                     struct funcref* input_to) {
  fiber->input_to = input_to;
}

static inline void fiber_set_accu(struct fiber* fiber, any value) {
  fiber->accu = value;
}

static inline any fiber_get_accu(struct fiber* fiber) {
  return fiber->accu;
}

static inline struct frame* fiber_top(struct fiber* fiber) {
  return fiber->top;
}

static inline void fiber_push(struct fiber* fiber, any a) {
  *((any*) fiber->sp) = a;
  fiber->sp += sizeof(any);
}

static inline any fiber_pop(struct fiber* fiber) {
  fiber->sp -= sizeof(any);
  return *((any*) fiber->sp);
}

static inline void fiber_drop(struct fiber* fiber, unsigned int count) {
  fiber->sp -= count * sizeof(any);
}

static inline any* fiber_stack_peek(struct fiber* fiber, unsigned int depth) {
  return (((any*) fiber->sp) - depth - 1);
}

static inline struct fiber_vars* fiber_vars(struct fiber* fiber) {
  return &fiber->vars;
}

#endif
