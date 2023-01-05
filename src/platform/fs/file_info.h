/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_FS_FILE_INFO_H
#define RAVEN_FS_FILE_INFO_H

#include "../../defs.h"

struct fs;

struct file_info {
    struct fs*          fs;
    struct file_info**  prev;
    struct file_info*   next;

    char*               virt_path;
    char*               real_path;

    struct blueprint*   blueprint;
    struct object*      object;

    raven_timestamp_t   last_compiled;
};

void file_info_create(struct file_info* info, struct fs* fs, const char* virt_path, const char* real_path);
void file_info_destroy(struct file_info* info);

struct file_info* file_info_new(struct fs* fs, const char* virt_path, const char* real_path);
void              file_info_delete(struct file_info* info);

void file_info_mark(struct gc* gc, struct file_info* info);

bool file_info_matches_virt(struct file_info* info, const char* virt_path);
bool file_info_matches_real(struct file_info* info, const char* real_path);

bool file_info_recompile_with_log(struct file_info* info, struct log* log);
bool file_info_recompile(struct file_info* info);

struct blueprint* file_info_blueprint(struct file_info* info, bool compile_if_missing);
struct object*    file_info_object(struct file_info* info, bool compile_if_missing);

raven_timestamp_t file_info_last_compiled(struct file_info* info);

bool              file_info_is_outdated(struct file_info* info);

#endif
