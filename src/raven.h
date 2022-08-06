#ifndef RAVEN_H
#define RAVEN_H

#include "defs.h"
#include "core/any.h"
#include "fs/filesystem.h"
#include "server/server.h"
#include "util/log.h"
#include "vm/scheduler.h"
#include "gc/gc.h"


struct raven_vars {
  any  nil_proxy;
  any  string_proxy;
  any  array_proxy;
  any  mapping_proxy;
  any  symbol_proxy;
};

struct raven {
  struct base_obj*   objects;
  struct symbol*     symbols;
  struct log         log;
  struct scheduler   scheduler;
  struct server      server;
  struct filesystem  fs;
  struct raven_vars  vars;
};

void raven_create(struct raven* raven);
void raven_destroy(struct raven* raven);

bool raven_boot(struct raven* raven, const char* mudlib);

void raven_mark(struct gc* gc, struct raven* raven);

void raven_run(struct raven* raven);

struct symbol* raven_find_symbol(struct raven* raven, const char* name);
struct symbol* raven_gensym(struct raven* raven);
struct blueprint* raven_get_blueprint(struct raven* raven, const char* path);
struct object* raven_get_object(struct raven* raven, const char* path);

void raven_setup_builtins(struct raven* raven);

static inline struct base_obj* raven_objects(struct raven* raven) {
  return raven->objects;
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

static inline struct filesystem* raven_fs(struct raven* raven) {
  return &raven->fs;
}

static inline struct raven_vars* raven_vars(struct raven* raven) {
  return &raven->vars;
}

#endif
