/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_FS_FILESYSTEM_H
#define RAVEN_FS_FILESYSTEM_H

#include "../defs.h"

struct filesystem {
    struct raven*  raven;
    struct file*   files;
    struct file*   root;
    const char*    anchor;
};

void filesystem_create(struct filesystem* fs, struct raven* raven);
void filesystem_destroy(struct filesystem* fs);

void filesystem_mark(struct gc* gc, struct filesystem* fs);

void filesystem_set_anchor(struct filesystem* fs, const char* anchor);
void filesystem_load(struct filesystem* fs);

struct file* filesystem_resolve(struct filesystem* fs, const char* path);
struct file* filesystem_resolve_flex(struct filesystem* fs, const char* path);

struct blueprint* filesystem_get_blueprint(struct filesystem* fs,
                                           const char* path);
struct object* filesystem_get_object(struct filesystem* fs, const char* path);

static inline struct raven* filesystem_raven(struct filesystem* fs) {
    return fs->raven;
}

static inline struct file* filesystem_root(struct filesystem* fs) {
    return fs->root;
}

static inline const char* filesystem_anchor(struct filesystem* fs) {
    return fs->anchor;
}

#endif
