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

void git_repo_create(struct git_repo* repo) {
    repo->path = NULL;
}

void git_repo_destroy(struct git_repo* repo) {
    memory_free(repo->path);
}

void git_repo_set_path(struct git_repo* repo, const char* path) {
    memory_free(repo->path);
    repo->path = memory_strdup(path);
}

bool git_repo_is_valid(struct git_repo* repo) {
    return (repo->path != NULL);
}

static bool git_repo_create_forker(struct git_repo* repo, struct forker* forker) {

    if (!git_repo_is_valid(repo))
        return false;

    forker_create(forker, "git");
    forker_add_arg(forker, "-C");
    forker_add_arg(forker, repo->path);
    forker_enable_wait(forker);

    return true;
}

bool git_repo_checkout(struct git_repo* repo, const char* branch, bool create) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "checkout");
        if (create) {
            forker_add_arg(&forker, "-b");
        }
        forker_add_arg(&forker, branch);
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}

bool git_repo_merge(struct git_repo* repo, const char* branch) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "merge");
        forker_add_arg(&forker, branch);
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}

bool git_repo_pull(struct git_repo* repo) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "pull");
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}

bool git_repo_fetch(struct git_repo* repo) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "fetch");
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}

bool git_repo_push(struct git_repo* repo) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "push");
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}

bool git_repo_stage_all(struct git_repo* repo) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "add");
        forker_add_arg(&forker, "-A");
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}

bool git_repo_commit(struct git_repo* repo, const char* message) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "commit");
        forker_add_arg(&forker, "-m");
        if (message == NULL)
            message = "No commit message provided.";
        forker_add_arg(&forker, message);
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}

bool git_repo_reset_hard(struct git_repo* repo) {
    struct forker         forker;
    bool                  success;

    success = false;

    if (git_repo_create_forker(repo, &forker)) {
        forker_add_arg(&forker, "reset");
        forker_add_arg(&forker, "--hard");
        success = forker_exec(&forker);
        forker_destroy(&forker);
    }

    return success;
}
