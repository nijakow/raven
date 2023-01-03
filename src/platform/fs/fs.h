/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_FS_FS_H
#define RAVEN_FS_FS_H

#include "../../defs.h"

struct file_info;

struct fs {
    struct raven*      raven;
    char*              anchor;
    struct file_info*  files;
};

void fs_create(struct fs* fs, struct raven* raven);
void fs_destroy(struct fs* fs);

void fs_set_anchor(struct fs* fs, const char* anchor);

void fs_mark(struct gc* gc, struct fs* fs);

bool fs_resolve(struct fs* fs, const char* path, const char* direction, struct stringbuilder* sb);
bool fs_normalize(struct fs* fs, const char* path, struct stringbuilder* sb);

bool fs_exists(struct fs* fs, const char* path);
bool fs_isdir(struct fs* fs, const char* path);

bool fs_read(struct fs* fs, const char* path, struct stringbuilder* sb);
bool fs_write(struct fs* fs, const char* path, const char* text);

struct file_info* fs_info(struct fs* fs, const char* path);

struct blueprint* fs_find_blueprint(struct fs* fs, const char* path, bool create);
struct object*    fs_find_object(struct fs* fs, const char* path, bool create);


typedef void (*fs_mapper_func)(void* data, const char* path);

bool fs_ls(struct fs* fs, const char* path, fs_mapper_func func, void* data);


static inline struct raven* fs_raven(struct fs* fs) {
    return fs->raven;
}

#endif
