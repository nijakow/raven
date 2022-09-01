/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "reader.h"

void reader_create(struct reader* reader, const char* source) {
  reader->position       = (char*) source;
  reader->src            = (char*) source;
  reader->file_pos.line  = 0;
  reader->file_pos.caret = 0;
}

void reader_destroy(struct reader* reader) {

}

bool reader_peekn(t_reader* reader, char* loc, const char* str) {
  unsigned int  i;

  i = 0;
  while (str[i] != '\0') {
    if (reader_peek(reader) == str[i]) {
      if (loc != NULL)
        *loc = reader_peek(reader);
      reader_advance(reader);
      return true;
    }
    i++;
  }
  return false;
}

bool reader_checkn(t_reader* reader, const char* str) {
  return reader_peekn(reader, NULL, str);
}

bool reader_checks(t_reader* reader, const char* str) {
  char*            backup;
  struct file_pos  backup_file_pos;
  unsigned int     i;

  backup          = reader->position;
  backup_file_pos = reader->file_pos;
  i               = 0;
  while (true) {
    if (str[i] == '\0') {
      return true;
    } else if (!reader_has(reader)) {
      return false;
    } else if ((str[i] != reader_advance(reader))) {
      reader->position = backup;
      reader->file_pos = backup_file_pos;
      return false;
    }
    i++;
  }
}

void reader_skip_whitespace(t_reader* reader) {
  while (reader_checkn(reader, " \t\r\n"));
}
