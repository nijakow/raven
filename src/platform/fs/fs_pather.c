/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../util/stringbuilder.h"

#include "fs_pather.h"


void fs_pather_create(struct fs_pather* pather) {
    pather->buffer[0] = '/';
    pather->write_head = 1;
}

void fs_pather_destroy(struct fs_pather* pather) {

}

void fs_pather_clear(struct fs_pather* pather) {
    pather->buffer[0] = '/';
    pather->write_head = 1;
}

bool fs_pather_is_slashed(struct fs_pather* pather) {
    return (pather->write_head > 0 && pather->buffer[pather->write_head - 1] == '/');
}

void fs_pather_append_char(struct fs_pather* pather, char c) {
    if (pather->write_head >= sizeof(pather->buffer))
        return;
    else if (!(c == '/' && fs_pather_is_slashed(pather))) {
        pather->buffer[pather->write_head] = c;
        pather->write_head++;
    }
}

void fs_pather_append_string(struct fs_pather* pather, const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        fs_pather_append_char(pather, str[i]);
    }
}

void fs_pather_unsafe_append(struct fs_pather* pather, const char* str) {
    fs_pather_append_string(pather, str);
}

void fs_pather_append_directory(struct fs_pather* pather, const char* dir) {
    fs_pather_append_char(pather, '/');
    fs_pather_append_string(pather, dir);
}

void fs_pather_up(struct fs_pather* pather) {
    while (pather->write_head > 0 && fs_pather_is_slashed(pather))
        pather->write_head--;
    while (pather->write_head > 0 && !fs_pather_is_slashed(pather))
        pather->write_head--;
}

void fs_pather_cd1(struct fs_pather* pather, const char* dir) {
    if (strcmp(dir, "..") == 0) {
        fs_pather_up(pather);
    } else if (strcmp(dir, ".") == 0 || strcmp(dir, "") == 0) {
        /*
         * Do nothing.
         */
    } else {
        fs_pather_append_directory(pather, dir);
    }
}

void fs_pather_cd(struct fs_pather* pather, const char* dir) {
    size_t  read_head;
    size_t  write_head;
    char    buffer[1024];

    if (dir[0] == '/') {
        fs_pather_clear(pather);
    }

    read_head  = 0;
    write_head = 0;

    while (true) {
        if (dir[read_head] == '/' || dir[read_head] == '\0') {
            buffer[write_head] = '\0';
            fs_pather_cd1(pather, buffer);
            write_head = 0;
            if (dir[read_head] == '\0')
                break;
        } else {
            buffer[write_head] = dir[read_head];
            write_head++;
        }
        read_head++;
    }
}

void fs_pather_write_out(struct fs_pather* pather, struct stringbuilder* sb) {
    size_t  i;

    for (i = 0; i < pather->write_head; i++) {
        stringbuilder_append_char(sb, pather->buffer[i]);
    }
}

const char* fs_pather_get_const(struct fs_pather* pather) {
    pather->buffer[pather->write_head] = '\0';
    return pather->buffer;
}
