/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_RINGBUFFER_H
#define RAVEN_UTIL_RINGBUFFER_H

#include "../defs.h"

#define RINGBUFFER_SIZE 1024

struct ringbuffer {
  unsigned int  lines;
  unsigned int  read_head;
  unsigned int  write_head;
  char          data[RINGBUFFER_SIZE];
};

void ringbuffer_create(struct ringbuffer* ringbuffer);
void ringbuffer_destroy(struct ringbuffer* ringbuffer);

bool ringbuffer_has(struct ringbuffer* ringbuffer);
bool ringbuffer_read(struct ringbuffer* ringbuffer, char* loc);
void ringbuffer_write(struct ringbuffer* ringbuffer, char c);

char* ringbuffer_line(struct ringbuffer* ringbuffer);

#endif
