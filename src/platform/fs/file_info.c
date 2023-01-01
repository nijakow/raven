/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../util/memory.h"

#include "fs.h"

#include "file_info.h"


void file_info_create(struct file_info* info, struct fs* fs, const char* virt_path) {
    info->fs        = fs;
    info->virt_path = memory_strdup(virt_path);

    info->prev      = &fs->files;
    info->next      = fs->files;

    if (fs->files != NULL) {
        fs->files->prev = &info->next;
    }

    info->blueprint = NULL;
    info->object    = NULL;
}

void file_info_destroy(struct file_info* info) {
    if (info->next != NULL) {
        info->next->prev = info->prev;
    }

    *info->prev = info->next;

    memory_free(info->virt_path);
}

struct file_info* file_info_new(struct fs* fs, const char* virt_path) {
    struct file_info* info;
    
    info = memory_alloc(sizeof(struct file_info));

    if (info != NULL) {
        file_info_create(info, fs, virt_path);
    }

    return info;
}

void file_info_delete(struct file_info* info) {
    if (info != NULL) {
        file_info_destroy(info);
        memory_free(info);
    }
}

bool file_info_matches(struct file_info* info, const char* virt_path) {
    return strcmp(info->virt_path, virt_path) == 0;
}

struct blueprint* file_info_blueprint(struct file_info* info, bool compile_if_missing) {
    if (info->blueprint != NULL)
        return info->blueprint;
    
    return NULL;  // TODO
}

struct object* file_info_object(struct file_info* info, bool compile_if_missing) {
    if (info->object != NULL)
        return info->object;
    
    return NULL;  // TODO
}
