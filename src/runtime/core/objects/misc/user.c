/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../../../defs.h"
#include "../../../../raven/raven.h"
#include "../../../../util/memory.h"

#include "user.h"

struct obj_info USER_INFO = {
    .type  = OBJ_TYPE_USER,
    .mark  = (mark_func)  user_mark,
    .del   = (del_func)   user_del,
    .stats = (stats_func) base_obj_stats
};

struct user* user_new(struct raven* raven, const char* name) {
    struct user*  user;

    user = base_obj_new(raven_objects(raven), &USER_INFO, sizeof(struct user));

    if (user != NULL) {
        user->name     = memory_strdup(name);
        user->password = memory_strdup("");
    }

    return user;
}

void user_mark(struct gc* gc, struct user* user) {
    base_obj_mark(gc, &user->_);
}

void user_del(struct user* user) {
    memory_free(user->name);
    memory_free(user->password);
    base_obj_del(&user->_);
}



bool user_compare_name(struct user* user, const char* name) {
    return strcmp(user->name, name) == 0;
}

bool user_compare_password(struct user* user, const char* password) {
    return strcmp(user->password, password) == 0;
}
