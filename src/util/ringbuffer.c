/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "stringbuilder.h"

#include "ringbuffer.h"

void ringbuffer_create(struct ringbuffer* ringbuffer) {
    ringbuffer->lines      = 0;
    ringbuffer->read_head  = 0;
    ringbuffer->write_head = 0;
}

void ringbuffer_destroy(struct ringbuffer* ringbuffer) {
}

bool ringbuffer_has(struct ringbuffer* ringbuffer) {
    return ringbuffer->read_head != ringbuffer->write_head;
}

static bool ringbuffer_has_space(struct ringbuffer* ringbuffer) {
    return (ringbuffer->write_head + 1) % RINGBUFFER_SIZE
        != ringbuffer->read_head;
}

bool ringbuffer_read(struct ringbuffer* ringbuffer, char* loc) {
    if (ringbuffer_has(ringbuffer)) {
        if (ringbuffer->data[ringbuffer->read_head] == '\n')
            ringbuffer->lines--;
        *loc = ringbuffer->data[ringbuffer->read_head];
        ringbuffer->read_head = (ringbuffer->read_head + 1) % RINGBUFFER_SIZE;
        return true;
    } else {
        return false;
    }
}

void ringbuffer_write(struct ringbuffer* ringbuffer, char c) {
    if (!ringbuffer_has_space(ringbuffer))
        return;
    if (c == '\n')
        ringbuffer->lines++;
    ringbuffer->data[ringbuffer->write_head] = c;
    ringbuffer->write_head = (ringbuffer->write_head + 1) % RINGBUFFER_SIZE;
}

char* ringbuffer_line(struct ringbuffer* ringbuffer) {
    char*                result;
    struct stringbuilder sb;
    char                 c;

    if (ringbuffer->lines == 0 && ringbuffer_has_space(ringbuffer))
        return NULL;

    stringbuilder_create(&sb);
    while (ringbuffer_read(ringbuffer, &c)) {
        if (c == '\n')
            break;
        else if (c == '\r');
        else {
            stringbuilder_append_char(&sb, c);
        }
    }
    stringbuilder_get(&sb, &result);
    stringbuilder_destroy(&sb);
    return result;
}
