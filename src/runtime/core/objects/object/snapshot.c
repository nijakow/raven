/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "snapshot.h"

void object_snapshot_page_create(struct object_snapshot_page* page) {
    page->next = NULL;
}

void object_snapshot_page_destroy(struct object_snapshot_page* page) {
    page->next = NULL;
}

void object_snapshot_page_construct_from_object_page(struct object_snapshot_page* page, struct object_page* object_page) {
    // TODO
}


void object_snapshot_create(struct object_snapshot* snapshot) {
    snapshot->prev = NULL;
    snapshot->next = NULL;

    snapshot->time_created = 0;
    snapshot->pages        = NULL;
}

void object_snapshot_destroy(struct object_snapshot* snapshot) {
    // TODO: Free all pages
}

void object_snapshot_push_page(struct object_snapshot* snapshot, struct object_snapshot_page* page) {
    raven_assert(page->next == NULL);

    page->next      = snapshot->pages;
    snapshot->pages = page;
}

void object_snapshot_construct_from_object(struct object_snapshot* snapshot, struct object* object) {
    // TODO
}
