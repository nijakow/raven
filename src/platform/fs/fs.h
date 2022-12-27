/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_FS_FS_H
#define RAVEN_FS_FS_H

#include "../../defs.h"

struct fs {
    struct raven*  raven;
    char*          anchor;
};

void fs_create(struct fs* fs, struct raven* raven, const char* anchor);
void fs_destroy(struct fs* fs);

bool fs_resolve(struct fs* fs, const char* path, const char* direction, struct stringbuilder* sb);

bool fs_exists(struct fs* fs, const char* path);
bool fs_isdir(struct fs* fs, const char* path);

#endif
