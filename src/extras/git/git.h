/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_EXTRAS_GIT_GIT_H
#define RAVEN_EXTRAS_GIT_GIT_H

#include "../../defs.h"

struct git_repo {
    char*  path;
};

void git_repo_create(struct git_repo* repo, const char* path);
void git_repo_destroy(struct git_repo* repo);

bool git_repo_is_valid(struct git_repo* repo);

bool git_repo_checkout(struct git_repo* repo, const char* branch);
bool git_repo_pull(struct git_repo* repo);
bool git_repo_push(struct git_repo* repo);

#endif
