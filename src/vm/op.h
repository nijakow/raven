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

bool fiber_op_eq(struct fiber* fiber, any a, any b);
bool fiber_op_ineq(struct fiber* fiber, any a, any b);

bool fiber_op_less(struct fiber* fiber, any a, any b);
bool fiber_op_leq(struct fiber* fiber, any a, any b);
bool fiber_op_greater(struct fiber* fiber, any a, any b);
bool fiber_op_geq(struct fiber* fiber, any a, any b);

any fiber_op_add(struct fiber* fiber, any a, any b);
any fiber_op_sub(struct fiber* fiber, any a, any b);
any fiber_op_mul(struct fiber* fiber, any a, any b);
any fiber_op_div(struct fiber* fiber, any a, any b);
any fiber_op_mod(struct fiber* fiber, any a, any b);

any fiber_op_negate(struct fiber* fiber, any a);

any fiber_op_bitand(struct fiber* fiber, any a, any b);
any fiber_op_bitor(struct fiber* fiber, any a, any b);

any fiber_op_leftshift(struct fiber* fiber, any a, any b);
any fiber_op_rightshift(struct fiber* fiber, any a, any b);

any fiber_op_index(struct fiber* fiber, any a, any b);
any fiber_op_index_assign(struct fiber* fiber, any a, any b, any c);
any fiber_op_deref(struct fiber* fiber, any a);
any fiber_op_sizeof(struct fiber* fiber, any a);
any fiber_op_new(struct fiber* fiber, any a);

bool fiber_op_typecheck(struct fiber* fiber, any a, struct type* type);
any  fiber_op_typecast(struct fiber* fiber, any a, struct type* type);

#endif
