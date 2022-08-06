#include "raven.h"

#include "core/objects/symbol.h"
#include "vm/builtins.h"
#include "vm/scheduler.h"


static void raven_create_vars(struct raven_vars* vars) {
  vars->nil_proxy     = any_nil();
  vars->string_proxy  = any_nil();
  vars->array_proxy   = any_nil();
  vars->mapping_proxy = any_nil();
  vars->symbol_proxy  = any_nil();
}

static void raven_mark_vars(struct gc* gc, struct raven_vars* vars) {
  gc_mark_any(gc, vars->nil_proxy);
  gc_mark_any(gc, vars->string_proxy);
  gc_mark_any(gc, vars->array_proxy);
  gc_mark_any(gc, vars->mapping_proxy);
  gc_mark_any(gc, vars->symbol_proxy);
}

void raven_create(struct raven* raven) {
  raven->objects = NULL;
  raven->symbols = NULL;
  log_create(&raven->log);
  scheduler_create(&raven->scheduler, raven);
  server_create(&raven->server, raven, 4242);
  filesystem_create(&raven->fs, raven);
  raven_setup_builtins(raven);
  raven_create_vars(&raven->vars);
}

void raven_destroy(struct raven* raven) {
  /* TODO: Collect all objects */
  filesystem_destroy(&raven->fs);
  server_destroy(&raven->server);
  scheduler_destroy(&raven->scheduler);
  log_destroy(&raven->log);
}

bool raven_boot(struct raven* raven, const char* mudlib) {
  filesystem_set_anchor(&raven->fs, mudlib);
  filesystem_load(&raven->fs);
  return true;
}

void raven_mark(struct gc* gc, struct raven* raven) {
  struct symbol*  symbol;

  for (symbol = raven->symbols; symbol != NULL; symbol = symbol->next)
    gc_mark_ptr(gc, symbol);
  raven_mark_vars(gc, raven_vars(raven));
  scheduler_mark(gc, raven_scheduler(raven));
  filesystem_mark(gc, raven_fs(raven));
}

void raven_gc(struct raven* raven) {
  struct gc  gc;

  gc_create(&gc, raven);
  gc_run(&gc);
  gc_destroy(&gc);
}

void raven_run(struct raven* raven) {
  unsigned int gc_steps;

  gc_steps = 0;
  while (true) {
    if (gc_steps++ % 128 == 0)
      raven_gc(raven);
    scheduler_run(raven_scheduler(raven));
    server_tick(raven_server(raven));
  }
}

struct symbol* raven_find_symbol(struct raven* raven, const char* name) {
  return symbol_find_in(raven, name);
}

struct symbol* raven_gensym(struct raven* raven) {
  return symbol_gensym(raven);
}

struct blueprint* raven_get_blueprint(struct raven* raven, const char* path) {
  return filesystem_get_blueprint(raven_fs(raven), path);
}

struct object* raven_get_object(struct raven* raven, const char* path) {
  return filesystem_get_object(raven_fs(raven), path);
}

static void raven_builtin(struct raven* raven,
                          const char*   name,
                          builtin_func  builtin) {
  symbol_set_builtin(raven_find_symbol(raven, name), builtin);
}

void raven_setup_builtins(struct raven* raven) {
  raven_builtin(raven, "_this_connection", builtin_this_connection);
  raven_builtin(raven, "print", builtin_print);
  raven_builtin(raven, "write", builtin_write);
  raven_builtin(raven, "write_to", builtin_write_to);
  raven_builtin(raven, "input_to", builtin_input_to);
  raven_builtin(raven, "input_line", builtin_input_line);

  raven_builtin(raven, "_the", builtin_the);
  raven_builtin(raven, "_initializedp", builtin_initializedp);
  raven_builtin(raven, "_initialize", builtin_initialize);

  raven_builtin(raven, "_arrayp", builtin_arrayp);
  raven_builtin(raven, "_stringp", builtin_stringp);
  raven_builtin(raven, "_objectp", builtin_objectp);
  raven_builtin(raven, "_functionp", builtin_functionp);

  raven_builtin(raven, "_nil_proxy", builtin_nil_proxy);
  raven_builtin(raven, "_string_proxy", builtin_string_proxy);
  raven_builtin(raven, "_array_proxy", builtin_array_proxy);
  raven_builtin(raven, "_mapping_proxy", builtin_mapping_proxy);
  raven_builtin(raven, "_symbol_proxy", builtin_symbol_proxy);

  raven_builtin(raven, "clone_object", builtin_clone_object);
  raven_builtin(raven, "_object_move", builtin_object_move);
  raven_builtin(raven, "_object_parent", builtin_object_parent);
  raven_builtin(raven, "_object_sibling", builtin_object_sibling);
  raven_builtin(raven, "_object_children", builtin_object_children);

  raven_builtin(raven, "_substr", builtin_substr);
  raven_builtin(raven, "mkarray", builtin_mkarray);
  raven_builtin(raven, "_append", builtin_append);
  raven_builtin(raven, "_chartostr", builtin_chartostr);
  raven_builtin(raven, "_keys", builtin_keys);

  raven_builtin(raven, "_isspace", builtin_isspace);
  raven_builtin(raven, "_wrap", builtin_wrap);

  raven_builtin(raven, "call", builtin_call);

  raven_builtin(raven, "this_player", builtin_this_player);

  raven_builtin(raven, "_ls", builtin_ls);
  raven_builtin(raven, "_resolve", builtin_resolve);
  raven_builtin(raven, "_cc", builtin_cc);
}
