/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "forker.h"

void forker_create(struct forker* forker, const char* executable) {
    charpp_create(&forker->args);
    charpp_create(&forker->env);
    charpp_append(&forker->args, executable);
}

void forker_destroy(struct forker* forker) {
    charpp_destroy(&forker->args);
    charpp_destroy(&forker->env);
}

void forker_add_arg(struct forker* forker, const char* arg) {
    charpp_append(&forker->args, arg);
}

void forker_add_env(struct forker* forker, const char* env) {
    charpp_append(&forker->env, env);
}

static bool forker_exec__check_path(const char* path) {
    return access(path, X_OK) == 0;
}

bool forker_exec(struct forker* forker) {
    if (!forker_exec__check_path(charpp_get_static_at(&forker->args, 0))) {
        return false;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        char*const* args = charpp_get_static(&forker->args);
        char*const* env  = charpp_get_static(&forker->env);
        execve(args[0], args, env);
        exit(127);
        return false;
    } else if (pid > 0) {
        // Parent process
        return true;
    } else {
        // Error
        return false;
    }
}
