/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../util/forker.h"
#include "../../util/memory.h"
#include "../../util/stringbuilder.h"

#include "git.h"

void git_repo_create(struct git_repo* repo, const char* path) {
    repo->path = memory_strdup(path);
}

void git_repo_destroy(struct git_repo* repo) {
    memory_free(repo->path);
}

bool git_repo_is_valid(struct git_repo* repo) {
    // TODO
    return true;
}

bool git_repo_checkout(struct git_repo* repo, const char* branch) {
    struct forker         forker;
    struct stringbuilder  sb;
    bool                  success;

    forker_create(&forker, "git");
    {
        stringbuilder_create(&sb);
        stringbuilder_append_str(&sb, "--git-dir=");
        stringbuilder_append_str(&sb, repo->path);
        forker_add_arg(&forker, stringbuilder_get_const(&sb));
        stringbuilder_destroy(&sb);
    }
    {
        stringbuilder_create(&sb);
        stringbuilder_append_str(&sb, "--work-tree=");
        stringbuilder_append_str(&sb, repo->path);
        forker_add_arg(&forker, stringbuilder_get_const(&sb));
        stringbuilder_destroy(&sb);
    }
    forker_add_arg(&forker, "checkout");
    forker_add_arg(&forker, branch);
    success = forker_exec(&forker);
    forker_destroy(&forker);

    return success;
}

bool git_repo_pull(struct git_repo* repo) {
    struct forker         forker;
    struct stringbuilder  sb;
    bool                  success;

    forker_create(&forker, "git");
    {
        stringbuilder_create(&sb);
        stringbuilder_append_str(&sb, "--git-dir=");
        stringbuilder_append_str(&sb, repo->path);
        forker_add_arg(&forker, stringbuilder_get_const(&sb));
        stringbuilder_destroy(&sb);
    }
    {
        stringbuilder_create(&sb);
        stringbuilder_append_str(&sb, "--work-tree=");
        stringbuilder_append_str(&sb, repo->path);
        forker_add_arg(&forker, stringbuilder_get_const(&sb));
        stringbuilder_destroy(&sb);
    }
    forker_add_arg(&forker, "pull");
    success = forker_exec(&forker);
    forker_destroy(&forker);

    return success;
}

bool git_repo_push(struct git_repo* repo) {
    struct forker         forker;
    struct stringbuilder  sb;
    bool                  success;

    forker_create(&forker, "git");
    {
        stringbuilder_create(&sb);
        stringbuilder_append_str(&sb, "--git-dir=");
        stringbuilder_append_str(&sb, repo->path);
        forker_add_arg(&forker, stringbuilder_get_const(&sb));
        stringbuilder_destroy(&sb);
    }
    {
        stringbuilder_create(&sb);
        stringbuilder_append_str(&sb, "--work-tree=");
        stringbuilder_append_str(&sb, repo->path);
        forker_add_arg(&forker, stringbuilder_get_const(&sb));
        stringbuilder_destroy(&sb);
    }
    forker_add_arg(&forker, "push");
    success = forker_exec(&forker);
    forker_destroy(&forker);

    return success;
}
