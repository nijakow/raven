/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "log.h"

void log_create(struct log* log) {
  /* TODO */
}

void log_destroy(struct log* log) {
  /* TODO */
}

void log_printf(struct log* log, const char* format, ...) {
  va_list  args;
  char     buffer[1024*16];

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  printf("[LOG]: %s", buffer);
  va_end(args);
}
