/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_OP_H
#define RAVEN_VM_OP_H

#include "../defs.h"
#include "../core/any.h"

bool raven_op_eq(struct fiber* fiber, any a, any b);
bool raven_op_ineq(struct fiber* fiber, any a, any b);

bool raven_op_less(struct fiber* fiber, any a, any b);
bool raven_op_leq(struct fiber* fiber, any a, any b);
bool raven_op_greater(struct fiber* fiber, any a, any b);
bool raven_op_geq(struct fiber* fiber, any a, any b);

any raven_op_add(struct fiber* fiber, any a, any b);
any raven_op_sub(struct fiber* fiber, any a, any b);
any raven_op_mul(struct fiber* fiber, any a, any b);
any raven_op_div(struct fiber* fiber, any a, any b);
any raven_op_mod(struct fiber* fiber, any a, any b);

any raven_op_negate(struct fiber* fiber, any a);

any raven_op_index(struct fiber* fiber, any a, any b);
any raven_op_index_assign(struct fiber* fiber, any a, any b, any c);
any raven_op_deref(struct fiber* fiber, any a);
any raven_op_sizeof(struct fiber* fiber, any a);

#endif
