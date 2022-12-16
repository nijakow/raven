/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "fs.h"

#include "../../util/stringbuilder.h"

void fs_create(struct fs* fs, struct raven* raven) {
    fs->raven = raven;
}

void fs_destroy(struct fs* fs) {

}


struct fs_write_buffer {
    size_t  write_head;
    char    buffer[1024];
};

void fs_write_buffer_create(struct fs_write_buffer* buffer) {
    buffer->write_head = 0;
}

void fs_write_buffer_destroy(struct fs_write_buffer* buffer) {

}

bool fs_write_buffer_is_slashed(struct fs_write_buffer* buffer) {
    return (buffer->write_head > 0 && buffer->buffer[buffer->write_head - 1] == '/');
}

void fs_write_buffer_append_char(struct fs_write_buffer* buffer, char c) {
    if (buffer->write_head >= sizeof(buffer->buffer))
        return;
    else if (!(c == '/' && fs_write_buffer_is_slashed(buffer))) {
        buffer->buffer[buffer->write_head] = c;
        buffer->write_head++;
    }
}

void fs_write_buffer_append_string(struct fs_write_buffer* buffer, const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        fs_write_buffer_append_char(buffer, str[i]);
    }
}

void fs_write_buffer_append_directory(struct fs_write_buffer* buffer, const char* dir) {
    fs_write_buffer_append_char(buffer, '/');
    fs_write_buffer_append_string(buffer, dir);
}

void fs_write_buffer_up(struct fs_write_buffer* buffer) {
    while (buffer->write_head > 0 && fs_write_buffer_is_slashed(buffer))
        buffer->write_head--;
    while (buffer->write_head > 0 && !fs_write_buffer_is_slashed(buffer))
        buffer->write_head--;
}

void fs_write_buffer_go(struct fs_write_buffer* buffer, const char* dir) {
    if (strcmp(dir, "..") == 0) {
        fs_write_buffer_up(buffer);
    } else if (strcmp(dir, ".") == 0 || strcmp(dir, "") == 0) {
        /*
         * Do nothing.
         */
    } else {
        fs_write_buffer_append_directory(buffer, dir);
    }
}

void fs_write_buffer_finish(struct fs_write_buffer* buffer, struct stringbuilder* sb) {
    size_t  i;

    for (i = 0; i < buffer->write_head; i++) {
        stringbuilder_append_char(sb, buffer->buffer[i]);
    }
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

void fs_read_buffer_step(struct fs_read_buffer* buffer, struct fs_write_buffer* write_buffer) {
    size_t  start;

    start = buffer->read_head;

    while (buffer->read_head <= buffer->write_head) {
        if (buffer->read_head >= buffer->write_head || buffer->buffer[buffer->read_head] == '/') {
            buffer->buffer[buffer->read_head] = '\0';
            fs_write_buffer_go(write_buffer, &buffer->buffer[start]);
            start = buffer->read_head + 1;
        }
        buffer->read_head++;
    }
}

bool fs_resolve(struct fs* fs, const char* path, const char* direction, struct stringbuilder* sb) {
    struct  fs_read_buffer  read_buffer;
    struct  fs_write_buffer write_buffer;

    /*
     * Read the direction into the read buffer.
     */
    fs_read_buffer_create(&read_buffer, direction);

    /*
     * Copy the path into the write buffer.
     */
    fs_write_buffer_create(&write_buffer);
    fs_write_buffer_append_string(&write_buffer, (direction[0] == '/') ? "" : path);

    /*
     * Step through the read buffer, appending to the write buffer.
     */
    while (fs_read_buffer_has(&read_buffer))
        fs_read_buffer_step(&read_buffer, &write_buffer);
    
    fs_write_buffer_finish(&write_buffer, sb);

    return true;
}

bool fs_normalize(struct fs* fs, const char* path, struct stringbuilder* sb) {
    return fs_resolve(fs, "/", path, sb);
}
