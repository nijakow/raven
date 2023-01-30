/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../../../util/memory.h"
#include "../../../../util/time.h"

#include "snapshot.h"

void object_snapshot_page_create(struct object_snapshot_page* page) {
    page->next = NULL;
}

void object_snapshot_page_destroy(struct object_snapshot_page* page) {
    page->next = NULL;
}

static struct object_snapshot_page* object_snapshot_page_new() {
    struct object_snapshot_page*  page;

    page = memory_alloc(sizeof(struct object_snapshot_page));

    if (page != NULL) {
        object_snapshot_page_create(page);
    }

    return page;
}

static void object_snapshot_page_delete(struct object_snapshot_page* page) {
    object_snapshot_page_destroy(page);
    memory_free(page);
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
    struct object_snapshot_page*  page;

    while (snapshot->pages != NULL) {
        page            = snapshot->pages;
        snapshot->pages = page->next;

        object_snapshot_page_delete(page);
    }
}

void object_snapshot_push_page(struct object_snapshot* snapshot, struct object_snapshot_page* page) {
    raven_assert(page->next == NULL);

    page->next      = snapshot->pages;
    snapshot->pages = page;
}

void object_snapshot_construct_from_object(struct object_snapshot* snapshot, struct object* object) {
    struct object_snapshot_page*  snapshot_page;
    struct object_page*           last_page;
    struct object_page*           page;

    snapshot->time_created = raven_now();

    /*
     * We need to iterate over the pages in reverse order of the
     * links in the chain. We have two options for this. We can
     * either use a recursive function (and risk stack overflows
     * in the process) or walk the list backwards by repeatedly
     * scanning through it from the beginning.
     * 
     * We use the second approach. It does not look very nice in
     * code, but it protects us from exploitation.
     */
    last_page = NULL;
    while (last_page != object->pages) {
        /*
         * We start at the beginning of the chain.
         */
        page = object->pages;

        /*
         * Iterate forwards until we find the next page.
         */
        while (page != NULL && page->next != last_page)
            page = page->next;
        
        /*
         * Got it!
         */
        last_page = page;

        if (page != NULL) {
            // TODO: Create the snapshot page and link it into the tree
            (void) snapshot_page;
        }
    }
}
