/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_PARSEPILER_H
#define RAVEN_LANG_PARSEPILER_H

#include "parser.h"
#include "../core/blueprint.h"

bool parsepile_file(struct parser* parser, struct blueprint* into);

#endif
