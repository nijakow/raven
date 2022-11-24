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
 * The public interface of this module exposes only two functions.
 */

#include "../core/blueprint.h"
#include "compiler.h"
#include "parser.h"

bool parsepile_script(struct parser* parser, struct compiler* compiler);
bool parsepile_file(struct parser* parser, struct blueprint* into);

#endif
