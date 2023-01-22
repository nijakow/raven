/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_H
#define RAVEN_H

#include "../defs.h"
#include "../util/log.h"

#include "../platform/fs/fs.h"
#include "../platform/server/server.h"

#include "../runtime/core/any.h"
#include "../runtime/core/type.h"
#include "../runtime/core/object_table.h"
#include "../runtime/core/objects/misc/user/users.h"

#include "../runtime/gc/gc.h"
#include "../runtime/vm/scheduler.h"

#include "../extras/git/git.h"

/*
 * All variables of type `any` that are associated with the state of
 * the MUD.
 */
struct raven_vars {
    any  nil_proxy;
    any  string_proxy;
    any  array_proxy;
    any  mapping_proxy;
    any  symbol_proxy;

    struct funcref*  connect_func;
    struct funcref*  disconnect_func;
};

/*
 * The main state of the MUD.
 *
 * This is the main structure that holds all the state and
 * substructures of the MUD.
 * 
 * Usually, this structure is allocated on the stack exactly once,
 * a pointer to this structure is then passed around to all the
 * functions that need to access to the main components of the driver.
 * 
 * Most subcomponents are designed to be largely independent of each other,
 * and can be used without the rest of the driver.
 * 
 * Some of them have a backpointer to the main structure, but this is
 * only for convenience. This allows us to find our way back to the main
 * structure from any subcomponent (which makes it work a lot like a
 * directory structure where you can `cd` into any subdirectory and then
 * `cd ..` back to the parent directory):
 * 
 *             +-------------------+
 *             |                   |
 *             V                   |
 *     +-----------------+         |
 *     | raven           |         |  backpointer to the main structure
 *     |                 |         |
 *     |   +--------------------+  |
 *     |   | subcomponent 1   *----+
 *     |   |                    |  |
 *     |   | internal state ... |  |
 *     |   +--------------------+  |
 *     |                 |         |
 *     |   +--------------------+  |
 *     |   | subcomponent 2   *----+
 *     |   |                    |
 *     |   | internal state ... |
 *     |   +--------------------+
 *     |                 |
 *     |   etc.          |
 *     |                 |
 *     +-----------------+
 * 
 */
struct raven {
    struct object_table  objects;
    struct typeset       types;
    struct log           log;
    struct scheduler     scheduler;
    struct server        server;
    struct fs            fs;
    struct git_repo      git;
    struct users         users;
    struct raven_vars    vars;
    bool                 was_interrupted;
};

void raven_create(struct raven* raven);
void raven_destroy(struct raven* raven);

void raven_mark(struct gc* gc, struct raven* raven);
void raven_gc(struct raven* raven);

bool raven_serve_on(struct raven* raven, int port);

void raven_interrupt(struct raven* raven);
void raven_loop(struct raven* raven);

bool raven_boot(struct raven* raven, const char* mudlib);
void raven_shutdown(struct raven* raven);
void raven_run(struct raven* raven);

struct symbol* raven_find_symbol(struct raven* raven, const char* name);
struct symbol* raven_gensym(struct raven* raven);

struct blueprint* raven_get_blueprint(struct raven* raven, const char* path, bool create);
struct object*    raven_get_object(struct raven* raven, const char* path, bool create);

bool  raven_recompile_with_log(struct raven* raven, const char* path, struct log* log);
bool  raven_recompile(struct raven* raven, const char* path);
bool  raven_recompile_object_with_log(struct raven* raven, struct object* object, struct log* log);
bool  raven_recompile_object(struct raven* raven, struct object* object);
bool  raven_refresh_object(struct raven* raven, struct object* object);

bool raven_call_out(struct raven* raven,
                    const char*   receiver,
                    const char*   name,
                    any*          args,
                    unsigned int  arg_count);
bool raven_call_out_func(struct raven*   raven,
                         struct funcref* funcref,
                         any*            args,
                         unsigned int    arg_count);

void raven_setup_builtins(struct raven* raven);

static inline struct object_table* raven_objects(struct raven* raven) {
    return &raven->objects;
}

static inline struct typeset* raven_types(struct raven* raven) {
    return &raven->types;
}

static inline struct log* raven_log(struct raven* raven) {
    return &raven->log;
}

static inline struct scheduler* raven_scheduler(struct raven* raven) {
    return &raven->scheduler;
}

static inline struct server* raven_server(struct raven* raven) {
    return &raven->server;
}

static inline struct fs* raven_fs(struct raven* raven) {
    return &raven->fs;
}

static inline struct git_repo* raven_git(struct raven* raven) {
    return &raven->git;
}

static inline struct users* raven_users(struct raven* raven) {
    return &raven->users;
}

static inline struct raven_vars* raven_vars(struct raven* raven) {
    return &raven->vars;
}

static inline bool raven_was_interrupted(struct raven* raven) {
    return raven->was_interrupted;
}

static inline raven_time_t raven_time(struct raven* raven) {
    (void) raven;
    return time(NULL);
}

#endif
