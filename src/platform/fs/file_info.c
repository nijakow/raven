/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../runtime/core/objects/object/object.h"
#include "../../runtime/core/blueprint.h"
#include "../../runtime/lang/parser.h"
#include "../../runtime/lang/parsepiler.h"
#include "../../util/memory.h"
#include "../../util/stringbuilder.h"
#include "../../util/time.h"

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

    fs->files       = info;

    info->blueprint = NULL;
    info->object    = NULL;

    info->last_compiled = 0;
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

void file_info_mark(struct gc* gc, struct file_info* info) {
    gc_mark_ptr(gc, info->blueprint);
    gc_mark_ptr(gc, info->object);
}

static struct raven* file_info_raven(struct file_info* info) {
    return fs_raven(info->fs);
}

bool file_info_matches_virt(struct file_info* info, const char* virt_path) {
    return strcmp(info->virt_path, virt_path) == 0;
}

bool file_info_matches_real(struct file_info* info, const char* real_path) {
    return strcmp(info->real_path, real_path) == 0;
}

static bool file_info_compile(struct file_info*  info,
                              const char*        source,
                              struct blueprint** loc,
                              struct log*        log) {
    struct reader      reader;
    struct parser      parser;
    struct blueprint*  blueprint;
    bool               result;

    log_printf(raven_log(file_info_raven(info)), "]%s\n", info->virt_path);

    result = false;

    blueprint = blueprint_new(file_info_raven(info), info->virt_path, info->real_path);

    reader_create(&reader, source);
    parser_create(&parser, file_info_raven(info), &reader, log);

    parser_set_file_name(&parser, info->real_path);

    if (parsepile_file(&parser, blueprint)) {
        if (loc != NULL)
            *loc = blueprint;
        info->last_compiled = raven_now();
        result = true;
    }

    parser_destroy(&parser);
    reader_destroy(&reader);

    return result;
}

bool file_info_recompile_with_log(struct file_info* info, struct log* log) {
    struct stringbuilder  contents;
    bool                  result;

    stringbuilder_create(&contents);
    result = fs_read(info->fs, info->real_path, &contents)
          && file_info_compile(info,
                               stringbuilder_get_const(&contents),
                              &info->blueprint,
                               log);
    stringbuilder_destroy(&contents);
    return result;
}

bool file_info_recompile(struct file_info* info) {
    return file_info_recompile_with_log(info, raven_log(file_info_raven(info)));
}

struct blueprint* file_info_blueprint(struct file_info* info, bool compile_if_missing) {
    if (info->blueprint == NULL && compile_if_missing) {
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

raven_timestamp_t file_info_last_compiled(struct file_info* info) {
    return info->last_compiled;
}

bool file_info_is_outdated(struct file_info* info) {
    raven_timestamp_t  last_modified;

    if (info->blueprint == NULL)
        return false;
    else if (!fs_last_modified(info->fs, info->real_path, &last_modified))
        return false;
    return raven_timestamp_less(info->last_compiled, last_modified);
}
