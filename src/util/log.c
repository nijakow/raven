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

void log_vprintf(struct log* log, const char* format, va_list args) {
  char  buffer[1024*16];

  vsnprintf(buffer, sizeof(buffer), format, args);
  printf("%s", buffer);
}

void log_printf(struct log* log, const char* format, ...) {
  va_list  args;

  va_start(args, format);
  log_vprintf(log, format, args);
  va_end(args);
}

void log_vprintf_error(struct log*  log,
                       const char*  src,
                       unsigned int line,
                       unsigned int caret,
                       const char*  format,
                       va_list      args)
{
  unsigned int cline;
  unsigned int ccaret;
  unsigned int index;
  unsigned int indent;

  cline  = 0;
  ccaret = 0;
  for (index = 0; src[index] != '\0'; index++) {
    if (cline == line)
      log_printf(log, "%c", src[index]);
    if (src[index] == '\n') {
      ccaret  = 0;
      cline  += 1;
    } else {
      ccaret += 1;
    }
    if (cline > line) break;
  }

  /*
   * If there was no newline, we add one.
   */
  if (cline == line) log_printf(log, "\n");

  /*
   * Indent the caret, and print the message.
   */
  for (indent = 0; indent < caret; indent++)
    log_printf(log, " ");
  log_printf(log, "^ [%u:%u]: ", line + 1, caret + 1);
  log_vprintf(log, format, args);
  log_printf(log, "\n");
}

void log_printf_error(struct log*  log,
                      const char*  src,
                      unsigned int line,
                      unsigned int caret,
                      const char*  format,
                      ...)
{
  va_list args;

  va_start(args, format);
  log_vprintf_error(log, src, line, caret, format, args);
  va_end(args);
}
