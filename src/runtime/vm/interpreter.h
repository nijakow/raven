/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_INTERPRETER_H
#define RAVEN_VM_INTERPRETER_H

#include "../../defs.h"

void fiber_send(struct fiber*  fiber,
                struct symbol* message,
                unsigned int   args);

void fiber_interpret(struct fiber* fiber);

#endif
