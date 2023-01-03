/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../util/memory.h"
#include "../../util/stringbuilder.h"

#include "file_info.h"
#include "fs_pather.h"

#include "fs.h"


void fs_create(struct fs* fs, struct raven* raven) {
    fs->raven  = raven;
    fs->anchor = memory_strdup("");
}

void fs_destroy(struct fs* fs) {
    memory_free(fs->anchor);
}

void fs_set_anchor(struct fs* fs, const char* anchor) {
    memory_free(fs->anchor);
    fs->anchor = memory_strdup(anchor);
}

void fs_mark(struct gc* gc, struct fs* fs) {
    // TODO, FIXME, XXX
}

struct fs_read_buffer {
    size_t  read_head;
    size_t  write_head;
    char    buffer[1024];
};

void fs_read_buffer_create(struct fs_read_buffer* buffer, const char* data) {
    buffer->read_head  = 0;
    buffer->write_head = 0;
    // TODO, FIXME, XXX: Check for length!
    while (data[buffer->write_head] != '\0') {
        buffer->buffer[buffer->write_head] = data[buffer->write_head];
        buffer->write_head++;
    }
}

void fs_read_buffer_destroy(struct fs_read_buffer* buffer) {

}

bool fs_read_buffer_has(struct fs_read_buffer* buffer) {
    return (buffer->read_head <= buffer->write_head);
}

void fs_read_buffer_step(struct fs_read_buffer* buffer, struct fs_pather* pather) {
    size_t  start;

    start = buffer->read_head;

    while (buffer->read_head <= buffer->write_head) {
        if (buffer->read_head >= buffer->write_head || buffer->buffer[buffer->read_head] == '/') {
            buffer->buffer[buffer->read_head] = '\0';
            fs_pather_cd(pather, &buffer->buffer[start]);
            start = buffer->read_head + 1;
        }
        buffer->read_head++;
    }
}

bool fs_resolve(struct fs* fs, const char* path, const char* direction, struct stringbuilder* sb) {
    struct  fs_read_buffer  read_buffer;
    struct  fs_pather       pather;

    /*
     * Read the direction into the read buffer.
     */
    fs_read_buffer_create(&read_buffer, direction);

    /*
     * Copy the path into the write buffer.
     */
    fs_pather_create(&pather);
    fs_pather_unsafe_append(&pather, (direction[0] == '/') ? "" : path);

    /*
     * Step through the read buffer, appending to the write buffer.
     */
    while (fs_read_buffer_has(&read_buffer))
        fs_read_buffer_step(&read_buffer, &pather);
    
    fs_pather_write_out(&pather, sb);

    return true;
}

bool fs_normalize(struct fs* fs, const char* path, struct stringbuilder* sb) {
    return fs_resolve(fs, "/", path, sb);
}

static bool fs_tofile(struct fs* fs, const char* virtpath, struct stringbuilder* sb) {
    stringbuilder_append_str(sb, fs->anchor);
    if (virtpath[0] != '/')
        stringbuilder_append_char(sb, '/');
    stringbuilder_append_str(sb, virtpath);
    return true;
}

bool fs_exists(struct fs* fs, const char* path) {
    struct stringbuilder   sb;
    struct stat            st;
    const char*            realpath;
    bool                   result;

    result = false;

    stringbuilder_create(&sb);
    if (fs_tofile(fs, path, &sb)) {
        realpath = stringbuilder_get_const(&sb);
        if (stat(realpath, &st) == 0) {
            result = true;
        }
    }
    stringbuilder_destroy(&sb);
    return result;
}

bool fs_isdir(struct fs* fs, const char* path) {
    struct stringbuilder   sb;
    struct stat            st;
    const char*            realpath;
    bool                   result;

    result = false;

    stringbuilder_create(&sb);
    if (fs_tofile(fs, path, &sb)) {
        realpath = stringbuilder_get_const(&sb);
        if (stat(realpath, &st) == 0) {
            result = S_ISDIR(st.st_mode) != 0;
        }
    }
    stringbuilder_destroy(&sb);
    return result;
}

bool fs_read(struct fs* fs, const char* path, struct stringbuilder* sb) {
    struct stringbuilder  sb2;
    FILE*                 file;
    size_t                byte;
    size_t                bytes_read;
    char                  buffer[1024];

    file = NULL;

    stringbuilder_create(&sb2);
    if (fs_tofile(fs, path, &sb2)) {
        file = fopen(stringbuilder_get_const(&sb2), "r");
        if (file != NULL) {
            while (true) {
                bytes_read = fread(buffer, 1, sizeof(buffer), file);
                if (bytes_read <= 0)
                    break;
                for (byte = 0; byte < bytes_read; byte++)
                    stringbuilder_append_char(sb, buffer[byte]);
            }
            fclose(file);
        }
    }
    stringbuilder_destroy(&sb2);

    return file != NULL;
}

bool fs_write(struct fs* fs, const char* path, const char* text) {
    // TODO, FIXME, XXX!
    return false;
}

struct file_info* fs_info(struct fs* fs, const char* path) {
    struct stringbuilder   sb;
    struct stringbuilder   sb2;
    struct file_info*      info;
    
    stringbuilder_create(&sb);
    {
        if (fs_normalize(fs, path, &sb)) {
            for (info = fs->files; info != NULL; info = info->next) {
                if (file_info_matches(info, stringbuilder_get_const(&sb)))
                    return info;
            }
        }

        info = NULL;

        {
            stringbuilder_create(&sb2);
            if (fs_tofile(fs, path, &sb2))
                info = file_info_new(fs, stringbuilder_get_const(&sb), stringbuilder_get_const(&sb2));
            stringbuilder_destroy(&sb2);
        }
    }
    stringbuilder_destroy(&sb);

    return info;
}

struct blueprint* fs_find_blueprint(struct fs* fs, const char* path, bool create) {
    struct file_info*   info;
    struct blueprint*   blueprint;

    blueprint = NULL;

    info = fs_info(fs, path);
    if (info != NULL) {
        blueprint = file_info_blueprint(info, create);
    }

    return blueprint;
}

struct object* fs_find_object(struct fs* fs, const char* path, bool create) {
    struct file_info*   info;
    struct object*      object;

    object = NULL;

    info = fs_info(fs, path);
    if (info != NULL) {
        object = file_info_object(info, create);
    }

    return object;
}

struct blueprint* fs_recompile(struct fs* fs, const char* path) {
    struct file_info*  info;

    info = fs_info(fs, path);

    if (info != NULL) {
        file_info_recompile(info);
        return file_info_blueprint(info, false);
    }

    return NULL;
}

bool fs_ls(struct fs* fs, const char* path, fs_mapper_func func, void* data) {
    DIR*                  dir;
    struct dirent*        entry;
    struct stringbuilder  sb;
    bool                  result;

    result = false;

    stringbuilder_create(&sb);
    {
        if (fs_normalize(fs, path, &sb)) {
            // Use dirent to list the directory.
            dir = opendir(stringbuilder_get_const(&sb));
            if (dir != NULL) {
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_name[0] != '.') {
                        func(data, entry->d_name);
                    }
                }
                closedir(dir);
                result = true;
            }
        }
    }
    stringbuilder_destroy(&sb);

    return result;
}
