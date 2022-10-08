/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "file.h"

#include "filesystem.h"


void filesystem_create(struct filesystem* fs, struct raven* raven) {
    fs->raven  = raven;
    fs->files  = NULL;
    fs->root   = NULL;
    fs->root   = file_new(fs, NULL, "");
    fs->anchor = NULL;
}

void filesystem_destroy(struct filesystem* fs) {
    while (fs->files != NULL)
        file_delete(fs->files);
}

void filesystem_mark(struct gc* gc, struct filesystem* fs) {
    struct file*  file;

    for (file = fs->files; file != NULL; file = file->next) {
        file_mark(gc, file);
    }
}

struct file* filesystem_resolve(struct filesystem* fs, const char* path) {
    return file_resolve(fs->root, path);
}

struct file* filesystem_resolve_flex(struct filesystem* fs, const char* path) {
    return file_resolve_flex(fs->root, path);
}

struct blueprint* filesystem_get_blueprint(struct filesystem* fs,
                                           const char* path) {
    struct file*  file;

    file = filesystem_resolve_flex(fs, path);
    if (file == NULL)
        return NULL;
    else {
        return file_get_blueprint(file);
    }
}

struct object* filesystem_get_object(struct filesystem* fs, const char* path) {
    struct file*  file;

    file = filesystem_resolve_flex(fs, path);
    if (file == NULL)
        return NULL;
    else {
        return file_get_object(file);
    }
}

void filesystem_set_anchor(struct filesystem* fs, const char* anchor) {
    fs->anchor = anchor;
}

void filesystem_load(struct filesystem* fs) {
    file_load(filesystem_root(fs), filesystem_anchor(fs));
}
