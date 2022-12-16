/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_OBJECT_OBJECT_H
#define RAVEN_OBJECTS_OBJECT_OBJECT_H

#include "../../../../defs.h"
#include "../../any.h"
#include "../../base_obj.h"

#include "page.h"

struct object {
    struct base_obj      _;
    struct object*       heartbeat_next;
    struct object**      heartbeat_prev;
    struct object*       parent;
    struct object*       sibling;
    struct object*       children;
    struct object_page*  pages;
    any                  stash;
    bool                 was_initialized;
};

struct object* object_new(struct raven* raven, struct blueprint* blueprint);
void           object_mark(struct gc* gc, struct object* object);
void           object_del(struct object* object);

struct object* object_clone(struct raven* raven, struct object* original);

void object_move_to(struct object* object, struct object* target);
void object_link_heartbeat(struct object* object, struct object** list);

void object_recompile(struct object* object);

static inline bool object_was_initialized(struct object* object) {
    return object->was_initialized;
}

static inline void object_set_initialized(struct object* object) {
    object->was_initialized = true;
}

static inline struct object* object_next_heartbeat(struct object* object) {
    return object->heartbeat_next;
}

static inline struct object* object_parent(struct object* object) {
    return object->parent;
}

static inline struct object* object_sibling(struct object* object) {
    return object->sibling;
}

static inline struct object* object_children(struct object* object) {
    return object->children;
}

static inline struct object_page* object_master_page(struct object* object) {
    return object->pages;
}

static inline struct blueprint* object_blueprint(struct object* object) {
    return object_page_blueprint(object_master_page(object));
}

static inline any object_stash(struct object* object) {
    return object->stash;
}

#endif
