/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "user.h"

#include "users.h"

void users_create(struct users* users, struct raven* raven) {
    users->raven = raven;
    users->list  = NULL;
}

void users_destroy(struct users* users) {
    assert(users->list == NULL);
}

struct user* users_find(struct users* users, const char* name) {
    struct user* user;
    
    user = users->list;
    while (user != NULL) {
        if (user_compare_name(user, name)) {
            return user;
        }
        user = user->next;
    }

    return user_new(users->raven, users, name);
}

struct user* users_login(struct users* users, const char* name, const char* password) {
    struct user* user;
    
    user = users_find(users, name);
    if (user != NULL && user_compare_password(user, password)) {
        return user;
    } else {
        return NULL;
    }
}
