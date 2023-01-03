/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_FS_FS_PATHER_H
#define RAVEN_FS_FS_PATHER_H

#include "../../defs.h"

struct fs_pather {
    size_t  write_head;
    char    buffer[1024];
};

void fs_pather_create(struct fs_pather* pather);
void fs_pather_destroy(struct fs_pather* pather);

void fs_pather_unsafe_append(struct fs_pather* pather, const char* str);

void fs_pather_cd(struct fs_pather* pather, const char* dir);

void fs_pather_write_out(struct fs_pather* pather, struct stringbuilder* sb);
const char* fs_pather_get_const(struct fs_pather* pather);

#endif
