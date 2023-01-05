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

/*
 * Set up a new file system.
 */
void fs_create(struct fs* fs, struct raven* raven) {
    fs->raven  = raven;
    fs->anchor = memory_strdup("");
}

/*
 * Destroy a file system.
 */
void fs_destroy(struct fs* fs) {
    memory_free(fs->anchor);
}

/*
 * Set the anchor of the file system.
 */
void fs_set_anchor(struct fs* fs, const char* anchor) {
    memory_free(fs->anchor);
    fs->anchor = memory_strdup(anchor);
}

/*
 * Run the garbage collector on the file system,
 * marking all cached blueprints and objects.
 */
void fs_mark(struct gc* gc, struct fs* fs) {
    struct file_info*  info;

    for (info = fs->files; info != NULL; info = info->next) {
        file_info_mark(gc, info);
    }
}


/*
 * The following section implements a "read buffer", which is a tool
 * for parsing paths. It is used by the fs_resolve function, and
 * helps us avoid making memory errors.
 * 
 * It will be moved to its own file in the future.
 */

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

/*
 * Resolve a path.
 *
 * This function resolves a path relative to the given path and writes the
 * result to the stringbuilder.
 * 
 * Returns false on failure.
 * 
 * This function is used to resolve paths, which is useful for finding relative
 * paths. This feature is used by the "include" command, and relative file
 * operations within the MUD.
 */
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

/*
 * Normalize a path.
 *
 * This function resolves a path relative to the root and writes the result to
 * the stringbuilder.
 * 
 * Returns false on failure.
 * 
 * This function is equivalent to fs_resolve(fs, "/", path, sb).
 * 
 * This function is used to normalize paths, such that they can be compared
 * against each other. For example, "/foo/bar" and "/foo/../foo/bar" are
 * equivalent paths, but they are not equal. This function will normalize both
 * paths to "/foo/bar".
 */
bool fs_normalize(struct fs* fs, const char* path, struct stringbuilder* sb) {
    return fs_resolve(fs, "/", path, sb);
}

/*
 * Convert a Raven path to a physical path.
 *
 * The physical path is written to the stringbuilder.
 * 
 * Returns false on failure.
 */
static bool fs_tofile(struct fs* fs, const char* virtpath, struct stringbuilder* sb) {
    stringbuilder_append_str(sb, fs->anchor);
    if (virtpath[0] != '/')
        stringbuilder_append_char(sb, '/');
    stringbuilder_append_str(sb, virtpath);
    return true;
}

/*
 * Check if a path exists.
 */
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

/*
 * Check if a path is a directory.
 */
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

/*
 * Get the last modified time of a file.
 */
bool fs_last_modified(struct fs* fs, const char* path, raven_timestamp_t* last_changed) {
    struct stringbuilder   sb;
    struct stat            st;
    const char*            realpath;
    bool                   result;

    result = false;

    stringbuilder_create(&sb);
    if (fs_tofile(fs, path, &sb)) {
        realpath = stringbuilder_get_const(&sb);
        if (stat(realpath, &st) == 0) {
            if (last_changed != NULL) {
                *last_changed = st.st_mtime;
            }
            result = true;
        }
    }
    stringbuilder_destroy(&sb);
    return result;
}

/*
 * This is a helper function for fs_read(...).
 */
bool fs_read__real(struct fs* fs, const char* path, struct stringbuilder* sb) {
    FILE*                 file;
    size_t                byte;
    size_t                bytes_read;
    char                  buffer[1024];

    file = fopen(path, "r");
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

    return file != NULL;
}

/*
 * Read a file into a stringbuilder.
 *
 * This function will first normalize the path, then read the file.
 * If the file does not exist, this function will return false.
 * 
 * This function uses the operating system's primitives to access
 * the physical file system. Therefore, a lot of care must be taken
 * to ensure that no injection attacks are possible.
 */
bool fs_read(struct fs* fs, const char* path, struct stringbuilder* sb) {
    struct stringbuilder  sb2;
    bool                  result;

    result = false;

    stringbuilder_create(&sb2);
    if (fs_tofile(fs, path, &sb2)) {
        result = fs_read__real(fs, stringbuilder_get_const(&sb2), sb);
    }
    stringbuilder_destroy(&sb2);

    return result;
}

bool fs_write(struct fs* fs, const char* path, const char* text) {
    // TODO, FIXME, XXX!
    return false;
}

/*
 * This is a helper function for fs_info(...).
 */
static struct file_info* fs_info__by_virt(struct fs* fs, const char* path, bool create) {
    struct stringbuilder   sb;
    struct stringbuilder   sb2;
    struct file_info*      info;
    
    stringbuilder_create(&sb);
    {
        if (fs_normalize(fs, path, &sb)) {
            for (info = fs->files; info != NULL; info = info->next) {
                if (file_info_matches_virt(info, stringbuilder_get_const(&sb)))
                    return info;
            }
        }

        info = NULL;

        if (create) {
            stringbuilder_create(&sb2);
            stringbuilder_append_str(&sb2, stringbuilder_get_const(&sb));
            stringbuilder_append_str(&sb2, ".lpc");
            info = file_info_new(fs, stringbuilder_get_const(&sb), stringbuilder_get_const(&sb2));
            stringbuilder_destroy(&sb2);
        }
    }
    stringbuilder_destroy(&sb);

    return info;
}

/*
 * This is a helper function for fs_info(...).
 */
static struct file_info* fs_info__by_real(struct fs* fs, const char* path, size_t dot_index, bool create) {
    struct stringbuilder   sb;
    struct stringbuilder   sb2;
    struct file_info*      info;
    size_t                 index;
    
    stringbuilder_create(&sb);
    {
        if (fs_normalize(fs, path, &sb)) {
            for (info = fs->files; info != NULL; info = info->next) {
                if (file_info_matches_real(info, stringbuilder_get_const(&sb)))
                    return info;
            }
        }

        info = NULL;

        if (create) {
            stringbuilder_create(&sb2);
            for (index = 0; index < dot_index; index++)
                stringbuilder_append_char(&sb2, path[index]);
            info = file_info_new(fs, stringbuilder_get_const(&sb2), stringbuilder_get_const(&sb));
            stringbuilder_destroy(&sb2);
        }
    }
    stringbuilder_destroy(&sb);

    return info;
}

/*
 * A helper function for fs_info(...).
 */
static struct file_info* fs_info__impl(struct fs* fs, const char* path, bool create) {
    size_t  index;

    for (index = 0; path[index] != '\0'; index++);
    while (index --> 0) {
        if (path[index] == '.')
            return fs_info__by_real(fs, path, index, create);
        else if (path[index] == '/')
            break;
    }
    return fs_info__by_virt(fs, path, create);
}

/*
 * Look up a file_info by path.
 *
 * There are two ways to look up a file_info:
 * 1. By virtual path. This is the path as it appears in the source code,
 *    for example, "/obj/room".
 * 2. By real path. This is the path as it appears on the filesystem,
 *    for example, "/obj/room.c".
 * 
 * In either case, the same file_info is returned.
 * 
 * It is possible to query the virtual and real path of a file_info,
 * regardless of how it was looked up.
 * 
 * A source for confusion is that the 'real' path doesn't contain the
 * file system's anchor. This is for security reasons. We only add the
 * anchor in the very last stage before calling the operating system's
 * file system primitives to avoid any possibility of a path traversal
 * attack.
 */
struct file_info* fs_info(struct fs* fs, const char* path) {
    return fs_info__impl(fs, path, true);
}

/*
 * Check if a file is loaded.
 *
 * This is done by looking up the file_info and then asking the file_info
 * if it is loaded.
 */
bool fs_is_loaded(struct fs* fs, const char* path) {
    return fs_info__impl(fs, path, false) != NULL;
}

/*
 * Check if a file is outdated.
 */
bool fs_is_outdated(struct fs* fs, const char* path) {
    struct file_info*  info;

    info = fs_info(fs, path);

    return info != NULL && file_info_is_outdated(info);
}

/*
 * Find a blueprint by path.
 *
 * This is done by looking up the file_info and then asking the file_info
 * for the blueprint.
 */
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

/*
 * Find an object by path.
 *
 * This is done by looking up the file_info and then asking the file_info
 * for the object.
 */
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

/*
 * Recompile a file.
 *
 * This is done by looking up the file_info and then asking the file_info
 * to recompile the file.
 */
struct blueprint* fs_recompile_with_log(struct fs* fs, const char* path, struct log* log) {
    struct file_info*  info;

    info = fs_info(fs, path);

    if (info != NULL) {
        file_info_recompile_with_log(info, log);
        return file_info_blueprint(info, false);
    }

    return NULL;
}

/*
 * Recompile a file.
 *
 * This is done by looking up the file_info and then asking the file_info
 * to recompile the file.
 */
struct blueprint* fs_recompile(struct fs* fs, const char* path) {
    struct file_info*  info;

    info = fs_info(fs, path);

    if (info != NULL) {
        file_info_recompile(info);
        return file_info_blueprint(info, false);
    }

    return NULL;
}


/*
 * List the contents of a directory.
 *
 * This function takes a pointer to the file system it should act upon,
 * followed by the desired path. After that, we have a function pointer
 * to a 'mapper' function. This function is called for each file in the
 * directory with the 'data' pointer as its first argument and the name
 * of the file as its second argument.
 * 
 * For now, dotfiles are ignored. This may change in the future.
 *
 * This is a file system primitive, therefore we take great care to
 * avoid any security issues. The path is normalized and then we
 * use the <dirent.h> functions to list the directory.
 */
bool fs_ls(struct fs* fs, const char* path, fs_mapper_func func, void* data) {
    DIR*                  dir;
    struct dirent*        entry;
    struct stringbuilder  sb;
    bool                  result;

    result = false;

    stringbuilder_create(&sb);
    {
        if (fs_tofile(fs, path, &sb)) {
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
