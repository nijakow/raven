/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_OBJECT_SNAPSHOT_H
#define RAVEN_OBJECTS_OBJECT_SNAPSHOT_H

#include "../../../../defs.h"

#include "object.h"


struct object_snapshot_page {
    struct object_snapshot_page*  next;
};

void object_snapshot_page_create(struct object_snapshot_page* page);
void object_snapshot_page_destroy(struct object_snapshot_page* page);

void object_snapshot_page_construct_from_object_page(struct object_snapshot_page* page, struct object_page* object_page);


struct object_snapshot {
    struct object_snapshot**      prev;
    struct object_snapshot*       next;

    raven_time_t                  time_created;

    struct object_snapshot_page*  pages;
};

void object_snapshot_create(struct object_snapshot* snapshot);
void object_snapshot_destroy(struct object_snapshot* snapshot);

void object_snapshot_push_page(struct object_snapshot* snapshot, struct object_snapshot_page* page);

void object_snapshot_construct_from_object(struct object_snapshot* snapshot, struct object* object);

#endif
