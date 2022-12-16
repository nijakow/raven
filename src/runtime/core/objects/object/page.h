/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_OBJECT_PAGE_H
#define RAVEN_OBJECTS_OBJECT_PAGE_H

#include "../../../../defs.h"
#include "../../any.h"

struct object_page {
    struct object*       object;
    struct object_page*  next;
    struct blueprint*    blue;
    unsigned int         slot_count;
    any*                 slots;
    any                  payload[];
};

void object_page_create(struct object_page* page, unsigned int slot_count, struct blueprint* blue);
void object_page_destroy(struct object_page* page);

void object_page_mark(struct gc* gc, struct object_page* page);
void object_page_del(struct object_page* page);

void object_page_link(struct object_page* page, struct object* object);

static inline struct object* object_page_object(struct object_page* page) {
    return page->object;
}

static inline struct blueprint* object_page_blueprint(struct object_page* page) {
    return page->blue;
}

static inline struct object_page* object_page_next(struct object_page* page) {
    return page->next;
}

static inline any* object_page_slot(struct object_page* page, unsigned int index) {
    return &page->slots[index];
}

static inline unsigned int object_page_slot_count(struct object_page* page) {
    return page->slot_count;
}

#endif
