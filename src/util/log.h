/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_LOG_H
#define RAVEN_UTIL_LOG_H

#include "../defs.h"

/*
 * This is a simple logging utility, which allows us to log messages to a
 * stringbuilder, or to stdout.
 * 
 * It provides a printf-like interface, and multiple output methods.
 * 
 * Most output in the driver is done through this utility, such that we can
 * intercept error messages in the compiler and redirect them to multiple
 * places (e.g. a player, the MUD's log, and some interested wizards).
 */
struct log {
    struct stringbuilder*  sb;
};

void log_create(struct log* log);
void log_create_to_stringbuilder(struct log* log, struct stringbuilder* sb);
void log_destroy(struct log* log);

void log_output_to_stringbuilder(struct log* log, struct stringbuilder* sb);

void log_putchar(struct log* log, char c);

void log_vprintf(struct log* log, const char* format, va_list args);
void log_printf(struct log* log, const char* format, ...);

void log_vprintf_error(struct log*  log,
                       const char*  name,
                       const char*  src,
                       unsigned int line,
                       unsigned int caret,
                       const char*  format,
                       va_list      args);
void log_printf_error(struct log*  log,
                      const char*  name,
                      const char*  src,
                      unsigned int line,
                      unsigned int caret,
                      const char*  format,
                      ...);

#endif
