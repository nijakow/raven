/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_EXTRAS_GIT_GIT_H
#define RAVEN_EXTRAS_GIT_GIT_H

#include "../../defs.h"

/*
 * Git is fun, so why not use it in your MUD?
 *
 * It happens a lot that some clown will try to break your MUD by
 * deleting files or modifying them. Recovering from this is a pain,
 * and it's even worse if you don't have backups.
 * 
 * Git is a version control system that allows you to keep track of
 * changes to your files, and revert them if something goes wrong,
 * which is exactly what we want.
 * 
 * Since Git has been gaining so much popularity in the last decade,
 * it is a safe bet that most people who are hosting a MUD driver
 * will have Git installed on their system.
 * 
 * This module provides a simple interface to Git, which allows you
 * to work on a repository, commit changes, and revert to previous
 * versions. It is designed to be used in conjunction with the
 * `fs` module, which provides a simple interface to the filesystem.
 * 
 * Since Raven aims to be free of heavy external dependencies, this
 * module is entirely optional and can be disabled. We also don't
 * link against the Git library, but instead use the `git` command
 * line tool. This means that you need to have Git installed on your
 * system and in your PATH.
 * 
 * THIS ALSO MEANS THAT WE ARE CALLING OUT TO AN EXTERNAL PROCESS
 * IN THE HOST FILE SYSTEM. THIS IS A SECURITY RISK, AND YOU SHOULD
 * PAY ATTENTION TO WHAT YOU ARE DOING.
 */
struct git_repo {
    char*  path;
};

void git_repo_create(struct git_repo* repo);
void git_repo_destroy(struct git_repo* repo);

void git_repo_set_path(struct git_repo* repo, const char* path);

bool git_repo_is_valid(struct git_repo* repo);

bool git_repo_checkout(struct git_repo* repo, const char* branch, bool create);
bool git_repo_merge(struct git_repo* repo, const char* branch);

bool git_repo_pull(struct git_repo* repo);
bool git_repo_fetch(struct git_repo* repo);
bool git_repo_push(struct git_repo* repo);

bool git_repo_stage_all(struct git_repo* repo);
bool git_repo_commit(struct git_repo* repo, const char* message);

bool git_repo_reset_hard(struct git_repo* repo);

#endif
