/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_LOG_H
#define RAVEN_UTIL_LOG_H

#include "../defs.h"

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
