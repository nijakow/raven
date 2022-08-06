/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_READER_H
#define RAVEN_LANG_READER_H

#include "../defs.h"

typedef char* t_reader;

static inline bool reader_has(t_reader* reader) {
  return **reader != '\0';
}

static inline char reader_peek(t_reader* reader) {
  return **reader;
}

static inline char reader_advance(t_reader* reader) {
  if (reader_has(reader))
    return *((*reader)++);
  return '\0';
}

static inline bool reader_check(t_reader* reader, char c) {
  if (reader_peek(reader) == c) {
    reader_advance(reader);
    return true;
  }
  return false;
}

bool reader_peekn(t_reader* reader, char* loc, const char* str);
bool reader_checkn(t_reader* reader, const char* str);
bool reader_checks(t_reader* reader, const char* str);

void reader_skip_whitespace(t_reader* reader);

#endif
