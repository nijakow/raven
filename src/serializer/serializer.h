/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_SERIALIZER_SERIALIZER_H
#define RAVEN_SERIALIZER_SERIALIZER_H

#include "../defs.h"

#include "../core/any.h"


enum serializer_tag {
    SERIALIZER_TAG_NONE,
    SERIALIZER_TAG_REF,
    SERIALIZER_TAG_NIL,
    SERIALIZER_TAG_INT,
    SERIALIZER_TAG_CHAR8,
    SERIALIZER_TAG_STRING,
    SERIALIZER_TAG_SYMBOL,
    SERIALIZER_TAG_ARRAY,
    SERIALIZER_TAG_MAPPING,
    SERIALIZER_TAG_FUNCREF,
    SERIALIZER_TAG_BLUEPRINT,
    SERIALIZER_TAG_OBJECT,
    SERIALIZER_TAG_ERROR = 0xff
};


struct serializer {

};

void serializer_create(struct serializer* serializer);
void serializer_destroy(struct serializer* serializer);

void serializer_write(struct serializer* serializer, const void* data, size_t size);
void serializer_write_with_size(struct serializer* serializer, const void* data, size_t size);

void serializer_write_uint8(struct serializer* serializer, uint8_t value);
void serializer_write_uint(struct serializer* serializer, uint32_t value);
void serializer_write_int(struct serializer* serializer, int32_t value);

void serializer_write_cstr(struct serializer* serializer, const char* str);

void serializer_write_tag(struct serializer* serializer, enum serializer_tag tag);

void serializer_write_ptr(struct serializer* serializer, void* obj);
void serializer_write_any(struct serializer* serializer, any any);

#endif
