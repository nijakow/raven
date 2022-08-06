#include "fiber.h"
#include "interpreter.h"

#include "scheduler.h"


void scheduler_create(struct scheduler* scheduler, struct raven* raven) {
  scheduler->raven  = raven;
  scheduler->fibers = NULL;
}

void scheduler_destroy(struct scheduler* scheduler) {
  while (scheduler->fibers != NULL)
    fiber_delete(scheduler->fibers);
}

void scheduler_mark(struct gc* gc, struct scheduler* scheduler) {
  struct fiber*  fiber;

  for (fiber = scheduler->fibers; fiber != NULL; fiber = fiber->next) {
    fiber_mark(gc, fiber);
  }
}

struct fiber* scheduler_new_fiber(struct scheduler* scheduler) {
  return fiber_new(scheduler);
}

void scheduler_run(struct scheduler* scheduler) {
  struct fiber** fiber;

  fiber = &scheduler->fibers;
  while (*fiber != NULL) {
    switch ((*fiber)->state)
    {
      case FIBER_STATE_RUNNING:
        fiber_interpret(*fiber);
        fiber = &((*fiber)->next);
        break;
      case FIBER_STATE_STOPPED:
      case FIBER_STATE_CRASHED:
        fiber_delete(*fiber);
        break;
      default:
        fiber = &((*fiber)->next);
        break;
    }
  }
}
