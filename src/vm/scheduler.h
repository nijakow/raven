/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_SCHEDULER_H
#define RAVEN_VM_SCHEDULER_H

/*
 * The Scheduler manages all the fibers in the system.
 * Every fiber gets inserted into a linked list.
 */

#include "../defs.h"

/*
 * The actual `scheduler` definition.
 */
struct scheduler {
  /*
   * A backpointer to the Raven instance.
   */
  struct raven*  raven;

  /*
   * A linked list of all the fibers registered for
   * this scheduler.
   */
  struct fiber*  fibers;
};

void scheduler_create(struct scheduler* scheduler, struct raven* raven);
void scheduler_destroy(struct scheduler* scheduler);

void scheduler_mark(struct gc* gc, struct scheduler* scheduler);

struct fiber* scheduler_new_fiber(struct scheduler* scheduler);

void scheduler_run(struct scheduler* scheduler);

static inline struct raven* scheduler_raven(struct scheduler* scheduler) {
  return scheduler->raven;
}

#endif
