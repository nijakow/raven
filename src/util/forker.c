/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "memory.h"
#include "stringbuilder.h"

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

static char* forker_exec__get_exec_path(const char* path_var, const char* executable) {
    struct stringbuilder  sb;
    size_t                index;
    char*                 result;

    if (path_var == NULL || executable == NULL)
        return NULL;
    else if (executable[0] == '/' || executable[0] == '.')
        return memory_strdup(executable);

    result = NULL;

    stringbuilder_create(&sb);
    {
        index = 0;

        do {
            if (path_var[index] == ':' || path_var[index] == '\0') {
                stringbuilder_append_char(&sb, '/');
                stringbuilder_append_str(&sb, executable);
                if (forker_exec__check_path(stringbuilder_get_const(&sb))) {
                    stringbuilder_get(&sb, &result);
                }
                stringbuilder_clear(&sb);
            } else {
                stringbuilder_append_char(&sb, path_var[index]);
            }
        } while (result == NULL && path_var[index++] != '\0');
    }
    stringbuilder_destroy(&sb);

    return result;
}

bool forker_exec(struct forker* forker) {
    char*  path;
    bool   result;

    result = false;

    path = forker_exec__get_exec_path(getenv("PATH"), charpp_get_static_at(&forker->args, 0));

    if (path == NULL)
        return false;

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        char*const* args = charpp_get_static(&forker->args);
        char*const* env  = charpp_get_static(&forker->env);
        execve(args[0], args, env);
        exit(127);
        result = false;
    } else if (pid > 0) {
        // Parent process
        result = true;
    } else {
        // Error
        result = false;
    }

    memory_free(path);

    return result;
}
