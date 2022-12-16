/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

 #include "../../../../defs.h"
 #include "../../../gc/gc.h"

 #include "page.h"

void object_page_create(struct object_page* page, unsigned int slot_count, struct blueprint* blue) {
    page->object      = NULL;
    page->next        = NULL;
    page->blue        = blue;
    page->slot_count  = slot_count;
    page->slots       = page->payload;
}

void object_page_destroy(struct object_page* page) {
    /*
     * Nothing to do.
     */
}

void object_page_mark(struct gc* gc, struct object_page* page) {
    unsigned int  index;

    gc_mark_ptr(gc, page->blue);
    for (index = 0; index < page->slot_count; index++)
        gc_mark_any(gc, page->slots[index]);
}

void object_page_del(struct object_page* page) {
    /*
     * TODO!
     */
}

void object_page_link(struct object_page* page, struct object* object) {
    // TODO!
}
