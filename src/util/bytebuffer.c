/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

#include "memory.h"

#include "bytebuffer.h"


void bytebuffer_create(struct bytebuffer* buffer) {
    buffer->alloc = 0;
    buffer->fill  = 0;
    buffer->data  = NULL;
}

void bytebuffer_destroy(struct bytebuffer* buffer) {
    if (buffer->data != NULL)
        memory_free(buffer->data);
}

static bool bytebuffer_resize(struct bytebuffer* buffer) {
    uint8_t*  data;

    if (buffer->data == NULL) {
        buffer->alloc = 32;
        buffer->data  = memory_alloc(32 * sizeof(uint8_t));
    } else {
        data = memory_alloc((buffer->alloc * 2) * sizeof(uint8_t));
        if (data == NULL)
            return false;
        memcpy(data, buffer->data, buffer->fill);
        memory_free(buffer->data);
        buffer->data  = data;
        buffer->alloc = buffer->alloc * 2;
    }
    return true;
}

void bytebuffer_write(struct bytebuffer* buffer, const void* data, size_t size) {
    if (buffer->fill + size >= buffer->alloc)
        if (!bytebuffer_resize(buffer))
            return;
    memcpy(buffer->data + buffer->fill, data, size);
    buffer->fill += size;
}

void bytebuffer_write_with_size(struct bytebuffer* buffer, const void* data, size_t size) {
    bytebuffer_write(buffer, &size, sizeof(size_t));
    bytebuffer_write(buffer, data, size);
}

void bytebuffer_write_uint8(struct bytebuffer* buffer, uint8_t value) {
    bytebuffer_write(buffer, &value, sizeof(uint8_t));
}

void bytebuffer_write_char(struct bytebuffer* buffer, char value) {
    bytebuffer_write(buffer, &value, sizeof(char));
}

void bytebuffer_write_uint(struct bytebuffer* buffer, unsigned int value) {
    bytebuffer_write(buffer, &value, sizeof(unsigned int));
}

void bytebuffer_write_int(struct bytebuffer* buffer, int value) {
    bytebuffer_write(buffer, &value, sizeof(int));
}
