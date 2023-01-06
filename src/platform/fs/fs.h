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


/*
 * This is Raven's file system abstraction. It is responsible for
 * resolving paths, reading files, and managing the file cache.
 * 
 * The file system is also responsible for managing the blueprint
 * information cache. Every time a file gets compiled, the blueprint
 * information is stored in the file system. This allows us to
 * retrieve the blueprint information for a file without having to
 * recompile it. Similarly, we also store object references in the
 * cache.
 * 
 * The file system is a component of the Raven object. It contains
 * an 'anchor', which is the name of the root directory of our
 * virtual file system (usually the directory containing the mudlib).
 * Every time a file is accessed, the file system will resolve the
 * path relative to the anchor.
 * 
 * For example, if the anchor is '/home/user/mudlib', and we try to
 * access the file '/foo/bar.lpc', the file system will resolve the
 * path to '/home/user/mudlib/foo/bar.lpc'.
 */
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

bool fs_last_modified(struct fs* fs, const char* path, raven_timestamp_t* last_changed);

bool fs_read(struct fs* fs, const char* path, struct stringbuilder* sb);
bool fs_write(struct fs* fs, const char* path, const char* text);

struct file_info* fs_info(struct fs* fs, const char* path);

bool              fs_is_loaded(struct fs* fs, const char* path);
bool              fs_is_outdated(struct fs* fs, const char* path);

struct blueprint* fs_find_blueprint(struct fs* fs, const char* path, bool create);
struct object*    fs_find_object(struct fs* fs, const char* path, bool create);

bool fs_recompile_with_log(struct fs* fs, const char* path, struct log* log);
bool fs_recompile(struct fs* fs, const char* path);

typedef void (*fs_mapper_func)(void* data, const char* path);

bool fs_ls(struct fs* fs, const char* path, fs_mapper_func func, void* data);


static inline struct raven* fs_raven(struct fs* fs) {
    return fs->raven;
}

#endif
