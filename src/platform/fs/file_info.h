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
};

void file_info_create(struct file_info* info, struct fs* fs, const char* virt_path);
void file_info_destroy(struct file_info* info);

struct file_info* file_info_new(struct fs* fs, const char* virt_path);
void              file_info_delete(struct file_info* info);

bool file_info_matches(struct file_info* info, const char* virt_path);

#endif
