/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_BYTEBUFFER_H
#define RAVEN_UTIL_BYTEBUFFER_H

#include "../defs.h"

struct bytebuffer {
    size_t    alloc;
    size_t    fill;
    uint8_t*  data;
};

void bytebuffer_create(struct bytebuffer* buffer);
void bytebuffer_destroy(struct bytebuffer* buffer);

void bytebuffer_write(struct bytebuffer* buffer, const void* data, size_t size);
void bytebuffer_write_with_size(struct bytebuffer* buffer, const void* data, size_t size);

void bytebuffer_write_uint8(struct bytebuffer* buffer, uint8_t value);

void bytebuffer_write_char(struct bytebuffer* buffer, char value);

void bytebuffer_write_uint(struct bytebuffer* buffer, unsigned int value);
void bytebuffer_write_int(struct bytebuffer* buffer, int value);

#endif
