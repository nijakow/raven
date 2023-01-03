/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"
#include "../../util/stringbuilder.h"
#include "../../util/memory.h"

#include "../../runtime/core/objects/object/object.h"
#include "../../runtime/core/blueprint.h"
#include "../../runtime/lang/parser.h"
#include "../../runtime/lang/parsepiler.h"

#include "filesystem.h"

#include "file.h"

struct file* file_new(struct filesystem* fs,
                      struct file*       parent,
                      const char*        name) {
    struct file*  file;
    size_t        name_len;

    name_len = strlen(name);

    file     = memory_alloc(sizeof(struct file) + (name_len + 1) * sizeof(char));

    if (file != NULL) {
        file->fs   =  fs;
        if (fs->files != NULL)
            fs->files->prev = &file->next;
        file->next =  fs->files;
        file->prev = &fs->files;
        fs->files  =  file;

        file->parent = parent;
        if (parent != NULL) {
            file->sibling    = parent->children;
            parent->children = file;
        }
        file->children  = NULL;

        file->blueprint = NULL;
        file->object    = NULL;

        memcpy(file->name, name, name_len);
        file->name[name_len] = '\0';
    }

    return file;
}

static void file_unlink(struct file* file) {
    struct file** ptr;

    if (file->parent == NULL)
        return;

    ptr = &file->parent->children;

    while (*ptr != NULL) {
        if (*ptr == file) {
            *ptr = file->sibling;
            break;
        }
        ptr = &((*ptr)->next);
    }
}

void file_delete(struct file* file) {
    while (file->children != NULL)
        file_delete(file->children);
    file_unlink(file);
    if (file->next != NULL)
        file->next->prev = file->prev;
    *(file->prev) = file->next;
    memory_free(file);
}


void file_mark(struct gc* gc, struct file* file) {
    gc_mark_ptr(gc, file->blueprint);
    gc_mark_ptr(gc, file->object);
}


static bool file_namecmp(const char* path, const char* fname) {
    unsigned int  i;

    i = 0;
    while (true) {
        if (fname[i] == '\0')
            return (path[i] == '\0' || path[i] == '/');
        else if (fname[i] != path[i])
            return false;
        i++;
    }
}

struct file* file_resolve1(struct file* file, const char* name) {
    struct file*  f;

    if (file_namecmp(name, "..")) return file->parent;
    else if (file_namecmp(name, ".")) return file;
    else {
        for (f = file->children; f != NULL; f = f->sibling) {
            if (file_namecmp(name, f->name))
                return f;
        }
    }

    return NULL;
}

struct file* file_resolve(struct file* file, const char* name) {
    unsigned int  i;

    if (file == NULL) return NULL;

    if (name[0] == '/') {
        return file_resolve(filesystem_root(file_fs(file)), &name[1]);
    }

    i = 0;
    while (file != NULL && name[i] != '\0') {
        file = file_resolve1(file, &name[i]);
        while (name[i] != '/' && name[i] != '\0')
            i++;
        if (name[i] != '\0')
            i++;
    }

    return file;
}

struct file* file_resolve_with_extension(struct file* file,
                                         const  char* path,
                                         const  char* extension) {
    unsigned int  index;
    char*         p;
    char          buffer[1024];

    if (file == NULL) return NULL;

    p = buffer;

    for (index = 0; path[index]      != '\0'; index++) *(p++) = path[index];
    for (index = 0; extension[index] != '\0'; index++) *(p++) = extension[index];
    *p = '\0';

    return file_resolve(file, buffer);
}

struct file* file_resolve_flex(struct file* file, const char* path) {
    struct file*  result;

    result = file_resolve_with_extension(file, path, ".lpc");
    if (result != NULL) return result;
    result = file_resolve_with_extension(file, path, ".c");
    if (result != NULL) return result;
    result = file_resolve_with_extension(file, path, ".h");
    if (result != NULL) return result;
    return file_resolve(file, path);
}

static bool file_write_path_helper(struct file* file, struct stringbuilder* sb) {
    if (file->parent != NULL) {
        if (!file_write_path_helper(file->parent, sb))
            return false;
        stringbuilder_append_str(sb, "/");
    }

    stringbuilder_append_str(sb, file->name);

    return true;
}

bool file_write_path(struct file* file, struct stringbuilder* sb) {
    if (file->parent == NULL) {
        stringbuilder_append_str(sb, "/");
        return true;
    } else {
        return file_write_path_helper(file, sb);
    }
}

char* file_path(struct file* file) {
    struct stringbuilder  sb;
    char*                 path;

    stringbuilder_create(&sb);
    file_write_path(file, &sb);
    stringbuilder_get(&sb, &path);
    stringbuilder_destroy(&sb);
    return path;
}

static bool file_open(struct file* file, FILE** fp, const char* mode) {
    struct stringbuilder  sb;

    stringbuilder_create(&sb);
    stringbuilder_append_str(&sb, filesystem_anchor(file_fs(file)));
    file_write_path(file, &sb);
    *fp = fopen(stringbuilder_get_const(&sb), mode);
    stringbuilder_destroy(&sb);
    return *fp != NULL;
}

bool file_is_directory(struct file* file) {
    struct stringbuilder  sb;
    struct stat           stats;
    bool                  result;

    result = false;

    stringbuilder_create(&sb);
    stringbuilder_append_str(&sb, filesystem_anchor(file_fs(file)));
    file_write_path(file, &sb);
    if (stat(stringbuilder_get_const(&sb), &stats) == 0) {
        result = S_ISDIR(stats.st_mode) ? true : false;
    }
    stringbuilder_destroy(&sb);
    return result;
}

bool file_read(struct file* file, struct stringbuilder* into) {
    FILE*   f;
    size_t  byte;
    size_t  bytes_read;
    char    buffer[1024];

    if (!file_open(file, &f, "r"))
        return false;
    while (1) {
        bytes_read = fread(buffer, sizeof(char), sizeof(buffer) - 1, f);
        if (bytes_read == 0) break;
        for (byte = 0; byte < bytes_read; byte++)
            stringbuilder_append_char(into, buffer[byte]);
    }
    fclose(f);
    return true;
}

bool file_write(struct file* file, const char* text) {
    FILE*   f;
    size_t  len;

    if (!file_open(file, &f, "w"))
        return false;
    len = strlen(text);
    fwrite(text, sizeof(char), len, f);
    fclose(f);
    return true;
}

bool file_recompile(struct file* file, struct log* log) {
    struct raven*         raven;
    struct reader         reader;
    struct parser         parser;
    struct blueprint*     blueprint;
    FILE*                 f;
    char*                 path;
    char*                 code;
    char*                 code_copy;
    bool                  result;
    size_t                byte;
    size_t                bytes_read;
    struct stringbuilder  sb;
    char                  buffer[1024];

    stringbuilder_create(&sb);
    file_write_path(file, &sb);
    log_printf(raven_log(filesystem_raven(file_fs(file))),
               "]%s\n",
               stringbuilder_get_const(&sb));
    stringbuilder_destroy(&sb);

    if (!file_open(file, &f, "r"))
        return false;

    stringbuilder_create(&sb);
    while (1) {
        bytes_read = fread(buffer, sizeof(char), sizeof(buffer) - 1, f);
        if (bytes_read == 0) break;
        for (byte = 0; byte < bytes_read; byte++)
            stringbuilder_append_char(&sb, buffer[byte]);
    }
    stringbuilder_get(&sb, &code);
    code_copy = code;
    stringbuilder_destroy(&sb);
    fclose(f);

    raven     = filesystem_raven(file->fs);
    blueprint = blueprint_new(raven, file);
    reader_create(&reader, code);
    parser_create(&parser, raven, &reader, log);
    path      = file_path(file);
    parser_set_file_name(&parser, path);
    result    = parsepile_file(&parser, blueprint);
    memory_free(path);
    parser_destroy(&parser);
    reader_destroy(&reader);
    memory_free(code_copy);

    if (result)
        file->blueprint = blueprint;

    return result;
}

struct blueprint* file_get_blueprint(struct file* file) {
    struct log  log;

    if (file->blueprint == NULL) {
        log_create(&log);
        file_recompile(file, &log);
        log_destroy(&log);
    }
    return file->blueprint;
}

struct object* file_get_object(struct file* file) {
    struct blueprint* blue;

    if (file->object == NULL) {
        blue = file_get_blueprint(file);
        if (blue != NULL) {
            file->object = object_new(filesystem_raven(file->fs), blue);
        }
    }

    return file->object;
}

bool file_load(struct file* file, const char* realpath) {
    DIR*                  dirp;
    struct dirent*        entry;
    struct stringbuilder  sb;
    char*                 path;
    struct file*          f;
    bool                  result;

    dirp = opendir(realpath);
    stringbuilder_create(&sb);
    result = false;

    if (dirp != NULL) {
        while ((entry = readdir(dirp)) != NULL) {
            if ((strcmp(entry->d_name, ".") != 0)
                && (strcmp(entry->d_name, "..") != 0)){
                stringbuilder_clear(&sb);
                stringbuilder_append_str(&sb, realpath);
                stringbuilder_append_char(&sb, '/');
                stringbuilder_append_str(&sb, entry->d_name);
                stringbuilder_get(&sb, &path);
                if (path != NULL) {
                    f = file_new(file_fs(file), file, entry->d_name);
                    if (f != NULL) {
                        file_load(f, path);
                        result = true;
                    }
                    memory_free(path);
                }
            }
        }

        closedir(dirp);
    }

    stringbuilder_destroy(&sb);
    return result;
}

struct blueprint* file_recompile_and_get(struct file* file) {
    struct filesystem*  fs;
    struct raven*       raven;

    fs    = file_fs(file);
    raven = filesystem_raven(fs);

    if (!file_recompile(file, raven_log(raven)))
        return NULL;

    return file_get_blueprint(file);
}
