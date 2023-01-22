/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_FIBER_H
#define RAVEN_VM_FIBER_H

/*
 * Fibers are to Raven what processes and threads are to
 * an operating system: They provide a way to run LPC code
 * concurrently.
 *
 * When the VM runs, it uses a linked list of fibers to
 * determine which ones to run, and then executes them for
 * a certain amount of time.
 *
 * A fiber holds the entire execution state of a Raven
 * process, including the stack frames, its capabilities
 * and limitations, some bookkeeping and scheduling details,
 * etc.
 */

#include "../../defs.h"
#include "../core/any.h"
#include "../core/objects/object/page.h"
#include "scheduler.h"

struct frame;

enum fiber_state {
    FIBER_STATE_RUNNING,
    FIBER_STATE_PAUSED,
    FIBER_STATE_SLEEPING,
    FIBER_STATE_WAITING_FOR_INPUT,
    FIBER_STATE_STOPPED,
    FIBER_STATE_CRASHED
};

struct fiber_vars {
    any              this_player;
    struct mapping*  fiber_locals;
    struct user*     effective_user;
};

struct fiber {
    any                accu;
    struct fiber*      next;
    struct fiber**     prev;
    enum fiber_state   state;
    struct scheduler*  scheduler;
    raven_time_t       wakeup_time;
    struct connection* connection;
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

void fiber_push_frame(struct fiber*       fiber,
                      struct object_page* page,
                      struct function*    func,
                      unsigned int        args);
void fiber_pop_frame(struct fiber* fiber);

void fiber_pause(struct fiber* fiber);
void fiber_wait_for_input(struct fiber* fiber);
void fiber_reactivate(struct fiber* fiber);
void fiber_reactivate_with_value(struct fiber* fiber, any value);
void fiber_throw(struct fiber* fiber, any value);
void fiber_do_crash(struct fiber* fiber,
                    const  char*  message,
                    const  char*  file,
                    int           line);
void fiber_unwind(struct fiber* fiber);

#define fiber_crash(fiber)                                              \
    fiber_do_crash(fiber, "fiber_crash(...) was called!", __FILE__, __LINE__)
#define fiber_crash_msg(fiber, msg)                 \
    fiber_do_crash(fiber, msg, __FILE__, __LINE__)

void fiber_print_backtrace(struct fiber* fiber, struct log* log);

void fiber_push_input(struct fiber* fiber, any value);

static inline struct raven* fiber_raven(struct fiber* fiber) {
    return scheduler_raven(fiber->scheduler);
}

static inline enum fiber_state fiber_state(struct fiber* fiber) {
    return fiber->state;
}

static inline void fiber_set_state(struct fiber*    fiber,
                                   enum fiber_state state) {
    fiber->state = state;
}

static inline struct connection* fiber_connection(struct fiber* fiber) {
    return fiber->connection;
}

static inline void fiber_set_connection(struct fiber* fiber,
                                        struct connection* connection) {
    fiber->connection = connection;
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

static inline struct mapping* fiber_locals(struct fiber* fiber) {
    return fiber_vars(fiber)->fiber_locals;
}

static inline void fiber_sleep_until(struct fiber* fiber, raven_time_t when) {
    fiber_set_state(fiber, FIBER_STATE_SLEEPING);
    fiber->wakeup_time = when;
}

static inline raven_time_t fiber_wakeup_time(struct fiber* fiber) {
    return fiber->wakeup_time;
}

#endif
