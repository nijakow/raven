/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"
#include "../core/objects/function.h"

#include "scheduler.h"
#include "frame.h"

#include "fiber.h"


static void fiber_create_vars(struct fiber_vars* vars) {
  vars->this_player  = any_nil();
}

void fiber_create(struct fiber* fiber, struct scheduler* scheduler) {
  fiber->state            =  FIBER_STATE_RUNNING;
  fiber->accu             =  any_nil();
  fiber->scheduler        =  scheduler;
  fiber->connection       =  NULL;
  fiber->input_to         =  NULL;
  fiber->sp               = &fiber->payload[0];
  fiber->top              =  NULL;

  fiber_create_vars(&fiber->vars);

  if (scheduler->fibers != NULL)
    scheduler->fibers->prev = &fiber->next;
  fiber->prev       = &scheduler->fibers;
  fiber->next       =  scheduler->fibers;
  scheduler->fibers =  fiber;
}

void fiber_destroy(struct fiber* fiber) {
  if (fiber->next != NULL)
    fiber->next->prev = fiber->prev;
  *fiber->prev = fiber->next;
}

struct fiber* fiber_new(struct scheduler* scheduler) {
  struct fiber*  fiber;

  fiber = malloc(sizeof(struct fiber) + (64 * 1024));

  if (fiber != NULL) {
    fiber_create(fiber, scheduler);
  }

  return fiber;
}

void fiber_delete(struct fiber* fiber) {
  fiber_destroy(fiber);
  free(fiber);
}

void fiber_mark(struct gc* gc, struct fiber* fiber) {
  char*          ptr;
  struct frame*  frame;

  frame = fiber->top;
  ptr   = fiber->sp;
  while (ptr > fiber->payload) {
    if (ptr == (char*) (frame + 1)) {
      ptr -= sizeof(struct frame);
      frame_mark(gc, frame);
      frame = frame->prev;
    } else {
      ptr -= sizeof(any);
      gc_mark_any(gc, *((any*) ptr));
    }
  }

  gc_mark_any(gc, fiber->accu);
  gc_mark_ptr(gc, fiber->input_to);
  gc_mark_any(gc, fiber->vars.this_player);
  gc_mark_ptr(gc, fiber_connection(fiber));
}

void fiber_push_frame(struct fiber*    fiber,
                      struct function* func,
                      unsigned int     args) {
  unsigned int   local_count;
  struct frame*  frame;
  any*           locals;
  int            missing; /* Must be signed! */

  local_count     = function_local_count(func);
  locals          = (any*) (fiber->sp - (args + 1) * sizeof(any));
  missing         = local_count - ((int) args + 1);
  frame           = (struct frame*) (fiber->sp + (missing * sizeof(any)));
  fiber->sp       = fiber->sp + (missing * sizeof(any)) + sizeof(struct frame);
  frame->prev     = fiber->top;
  frame->function = func;
  frame->ip       = 0;
  frame->locals   = locals;
  fiber->top      = frame;
  /* Initialize all uninitialized variables */
  while (local_count --> args + 1)
    locals[local_count] = any_nil();
}

void fiber_pop_frame(struct fiber* fiber) {
  fiber->sp  = (char*) fiber->top->locals;
  fiber->top =         fiber->top->prev;

  if (fiber->top == NULL) {
    fiber->state = FIBER_STATE_STOPPED;
  }
}

void fiber_pause(struct fiber* fiber) {
  fiber->state = FIBER_STATE_PAUSED;
}

void fiber_wait_for_input(struct fiber* fiber) {
  fiber->state = FIBER_STATE_WAITING_FOR_INPUT;
}

void fiber_reactivate(struct fiber* fiber) {
  fiber->state = FIBER_STATE_RUNNING;
}

void fiber_reactivate_with_value(struct fiber* fiber, any value) {
  fiber_set_accu(fiber, value);
  fiber_reactivate(fiber);
}

void fiber_do_crash(struct fiber* fiber, const char* file, int line) {
  fiber->state = FIBER_STATE_CRASHED;
  printf("Crash! %s %d\n", file, line);
}

void fiber_push_input(struct fiber* fiber, struct string* text) {
  if (fiber->state == FIBER_STATE_WAITING_FOR_INPUT) {
    fiber_reactivate_with_value(fiber, any_from_ptr(text));
  }
}
