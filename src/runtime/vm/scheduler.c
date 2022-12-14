/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../raven/raven.h"

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

bool scheduler_is_sleeping(struct scheduler* scheduler) {
    struct fiber*  fiber;

    for (fiber = scheduler->fibers; fiber != NULL; fiber = fiber->next) {
        if (fiber_state(fiber) == FIBER_STATE_RUNNING)
            return false;
    }

    return true;
}

void scheduler_run(struct scheduler* scheduler) {
    struct fiber**  fiber;
    raven_time_t    current_time;

    current_time = raven_time(scheduler_raven(scheduler));

    fiber = &scheduler->fibers;
    while (*fiber != NULL) {
        switch (fiber_state(*fiber))
        {
        case FIBER_STATE_RUNNING:
            fiber_interpret(*fiber);
            fiber = &((*fiber)->next);
            break;
        case FIBER_STATE_SLEEPING:
            if (current_time >= fiber_wakeup_time(*fiber)) {
                fiber_set_state(*fiber, FIBER_STATE_RUNNING);
            }
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
