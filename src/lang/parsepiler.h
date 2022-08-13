/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_PARSEPILER_H
#define RAVEN_LANG_PARSEPILER_H

/*
 * The Parsepiler is a combination of a parser and a compiler.
 * Our LPC code gets parsed and compiled into bytecode in one
 * go, there are no intermediate steps.
 *
 * The public interface of this module exposes only one function.
 */

#include "parser.h"
#include "../core/blueprint.h"

bool parsepile_file(struct parser* parser, struct blueprint* into);

#endif
