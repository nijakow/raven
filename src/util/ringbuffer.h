/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_RINGBUFFER_H
#define RAVEN_UTIL_RINGBUFFER_H

#include "../defs.h"

/*
 * Raven uses the `read()` and `write()` system calls for
 * all I/O with the clients. Since `read()` and `write()`
 * work on bytes and not on lines, we need a way to convert
 * the incoming bytes into full lines before we can forward
 * them to the LPC code.
 *
 * Ringbuffers help us with this task. A ringbuffer consists
 * of a char array that holds the data, and two pointers,
 * one of which is called the READ HEAD, the other one is
 * called the WRITE HEAD:
 *
 *
 *                 ....
 *               .      .
 *              .        .
 *              .        . <- READ
 *               .      .
 *                 ....
 *                  ^
 *                  |
 *                  W
 *                  R
 *                  I
 *                  T
 *                  E
 *
 * When the ringbuffer gets created, both the READ and the
 * WRITE head point to the same element in the `data` array.
 * Which one doesn't matter: As long as (READ == WRITE) the
 * ringbuffer is considered to be empty.
 *
 * After a character gets inserted, we advance the WRITE head
 * by one. Reading works the same way: Fetch the current
 * character under the READ pointer, and increment READ by one.
 * Whenever a pointer shoots over the size of the underlying
 * data buffer, we set it back to zero. This way, the buffer
 * looks like it's circular.
 *
 * If we keep a counter of how many '\n' we have in our buffer,
 * fetching the next line becomes easy:
 *
 *    void print_line(struct ringbuffer* rb) {
 *        char c;
 *
 *        c = ringbuffer_read(rb);
 *
 *        while (c != '\n') {
 *            printf("%c", c);
 *            c = ringbuffer_read(rb);
 *        }
 *    }
 *
 * There are rare cases where the WRITE head makes a full turn
 * and writes over the READ head. In this case, the previous
 * input is of course lost. This usually shouldn't happen,
 * but it's not as bad as a buffer overflow into unsafe
 * memory.
 */

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
