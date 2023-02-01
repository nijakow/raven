/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_SERIALIZER_SERIALIZER_H
#define RAVEN_SERIALIZER_SERIALIZER_H

#include "../../defs.h"

#include "../../util/bytebuffer.h"

#include "../core/any.h"


enum serializer_tag {
    SERIALIZER_TAG_NONE,
    SERIALIZER_TAG_LABEL,
    SERIALIZER_TAG_REF,
    SERIALIZER_TAG_NIL,
    SERIALIZER_TAG_INT,
    SERIALIZER_TAG_CHAR8,
    SERIALIZER_TAG_RUNE,
    SERIALIZER_TAG_STRING,
    SERIALIZER_TAG_SYMBOL,
    SERIALIZER_TAG_ARRAY,
    SERIALIZER_TAG_MAPPING,
    SERIALIZER_TAG_FUNCREF,
    SERIALIZER_TAG_BLUEPRINT,
    SERIALIZER_TAG_OBJECT,
    SERIALIZER_TAG_ERROR = 0xff
};


struct serializer_object_page_entry {
    uint32_t  id;
    void*     value;
};


struct serializer_object_page {
    struct serializer_object_page*       next;
    unsigned int                         fill;
    struct serializer_object_page_entry  entries[256];
};

struct serializer_object_page* serializer_object_page_new();
void                           serializer_object_page_delete(struct serializer_object_page* page);

bool serializer_object_page_add(struct serializer_object_page* page, void* obj, uint32_t id);


struct serializer_object_pages {
    struct serializer_object_page*  first;
    uint32_t                        next_id;
};

void serializer_object_pages_create(struct serializer_object_pages* pages);
void serializer_object_pages_destroy(struct serializer_object_pages* pages);

bool     serializer_object_pages_add(struct serializer_object_pages* pages, void* obj, uint32_t* id);
bool     serializer_object_pages_lookup(struct serializer_object_pages* pages, void* obj, uint32_t* id);


struct serializer {
    struct serializer_object_pages  object_pages;
    struct bytebuffer*              buffer;
};

void serializer_create(struct serializer* serializer);
void serializer_destroy(struct serializer* serializer);

void serializer_setup_write_to_bytebuffer(struct serializer* serializer, struct bytebuffer* buffer);

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
