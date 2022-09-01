/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_READER_H
#define RAVEN_LANG_READER_H

#include "../defs.h"

struct file_pos {
  unsigned int  line;
  unsigned int  caret;
};

typedef struct reader {
  char*            position;
  char*            src;
  struct file_pos  file_pos;
} t_reader;

void reader_create(struct reader* reader, const char* source);
void reader_destroy(struct reader* reader);

static inline char** reader_ptr(t_reader* reader) {
  return &reader->position;
}

static inline char* reader_get(t_reader* reader) {
  return reader->position;
}

static inline struct file_pos reader_file_pos(t_reader* reader) {
  return reader->file_pos;
}

static inline const char* reader_src(t_reader* reader) {
  return reader->src;
}

static inline unsigned int reader_line(t_reader* reader) {
  return reader->file_pos.line;
}

static inline unsigned int reader_caret(t_reader* reader) {
  return reader->file_pos.caret;
}

static inline bool reader_has(t_reader* reader) {
  return *reader_get(reader) != '\0';
}

static inline char reader_peek(t_reader* reader) {
  return *reader_get(reader);
}

static inline char reader_advance(t_reader* reader) {
  if (reader_has(reader)) {
    if (reader_peek(reader) == '\n') {
      reader->file_pos.caret  = 0;
      reader->file_pos.line  += 1;
    } else {
      reader->file_pos.caret++;
    }
    return *((*reader_ptr(reader))++);
  }
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
