/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../runtime/core/objects/object/object.h"
#include "../../util/memory.h"

#include "fs.h"

#include "file_info.h"


void file_info_create(struct file_info* info,
                      struct fs*        fs,
                      const char*       virt_path,
                      const char*       real_path) {
    info->fs        = fs;
    info->virt_path = memory_strdup(virt_path);
    info->real_path = memory_strdup(real_path);

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
    memory_free(info->real_path);
}

struct file_info* file_info_new(struct fs* fs, const char* virt_path, const char* real_path) {
    struct file_info* info;
    
    info = memory_alloc(sizeof(struct file_info));

    if (info != NULL) {
        file_info_create(info, fs, virt_path, real_path);
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

bool file_info_recompile(struct file_info* info) {
    return false;  // TODO
}

struct blueprint* file_info_blueprint(struct file_info* info, bool compile_if_missing) {
    if (info->blueprint == NULL) {
        file_info_recompile(info);
    }

    return info->blueprint;
}

struct object* file_info_object(struct file_info* info, bool compile_if_missing) {
    struct blueprint* blueprint;

    if (info->object == NULL) {
        blueprint = file_info_blueprint(info, compile_if_missing);

        if (blueprint != NULL) {
            info->object = object_new(fs_raven(info->fs), blueprint);
        }
    }
    
    return info->object;
}
