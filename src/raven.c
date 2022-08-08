/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "raven.h"

#include "core/objects/symbol.h"
#include "vm/builtins.h"
#include "vm/scheduler.h"


/*
 * Make all of Raven's LPC vars point to `nil`.
 */
static void raven_create_vars(struct raven_vars* vars) {
  vars->nil_proxy     = any_nil();
  vars->string_proxy  = any_nil();
  vars->array_proxy   = any_nil();
  vars->mapping_proxy = any_nil();
  vars->symbol_proxy  = any_nil();
}

/*
 * Create a new instance of Raven.
 */
void raven_create(struct raven* raven) {
  raven->objects = NULL;
  raven->symbols = NULL;
  typeset_create(&raven->types);
  log_create(&raven->log);
  scheduler_create(&raven->scheduler, raven);
  server_create(&raven->server, raven, 4242);
  filesystem_create(&raven->fs, raven);
  raven_setup_builtins(raven);
  raven_create_vars(&raven->vars);
}

/*
 * Destroy an instance of Raven.
 */
void raven_destroy(struct raven* raven) {
  /* TODO: Collect all objects */
  filesystem_destroy(&raven->fs);
  server_destroy(&raven->server);
  scheduler_destroy(&raven->scheduler);
  log_destroy(&raven->log);
}

/*
 * Load a Raven mudlib, and prepare all important objects.
 */
bool raven_boot(struct raven* raven, const char* mudlib) {
  filesystem_set_anchor(&raven->fs, mudlib);
  filesystem_load(&raven->fs);
  return true;
}

/*
 * Mark all of Raven's LPC vars during a garbage collection.
 */
static void raven_mark_vars(struct gc* gc, struct raven_vars* vars) {
  gc_mark_any(gc, vars->nil_proxy);
  gc_mark_any(gc, vars->string_proxy);
  gc_mark_any(gc, vars->array_proxy);
  gc_mark_any(gc, vars->mapping_proxy);
  gc_mark_any(gc, vars->symbol_proxy);
}

/*
 * Mark the Raven state during a garbage collection.
 */
void raven_mark(struct gc* gc, struct raven* raven) {
  struct symbol*  symbol;

  for (symbol = raven->symbols; symbol != NULL; symbol = symbol->next)
    gc_mark_ptr(gc, symbol);
  raven_mark_vars(gc, raven_vars(raven));
  scheduler_mark(gc, raven_scheduler(raven));
  filesystem_mark(gc, raven_fs(raven));
}

/*
 * Start a garbage collection on a Raven instance.
 */
void raven_gc(struct raven* raven) {
  struct gc  gc;

  gc_create(&gc, raven);
  gc_run(&gc);
  gc_destroy(&gc);
}

/*
 * The main loop.
 */
void raven_run(struct raven* raven) {
  unsigned int gc_steps;

  /*
   * We count the amount of loop iterations until we trigger the
   * next garbage collection. This variable holds the amount of steps.
   */
  gc_steps = 0;

  /*
   * This is the main loop of the MUD. It consists of a call to the
   * scheduler (which in turn advances all the running game threads),
   * followed by asking the socket server to update the MUD's
   * connections and sessions.
   */
  while (true) {
    if (gc_steps++ % 128 == 0)
      raven_gc(raven);
    scheduler_run(raven_scheduler(raven));
    server_tick(raven_server(raven));
  }
}

/*
 * Look up a symbol by its name inside of Raven's symbol table.
 */
struct symbol* raven_find_symbol(struct raven* raven, const char* name) {
  return symbol_find_in(raven, name);
}

/*
 * Generate a new, globally unique symbol.
 */
struct symbol* raven_gensym(struct raven* raven) {
  return symbol_gensym(raven);
}

/*
 * Resolve a blueprint by its path.
 */
struct blueprint* raven_get_blueprint(struct raven* raven, const char* path) {
  return filesystem_get_blueprint(raven_fs(raven), path);
}

/*
 * Resolve an object by its path.
 */
struct object* raven_get_object(struct raven* raven, const char* path) {
  return filesystem_get_object(raven_fs(raven), path);
}

/*
 * Declare a new builtin.
 * Used only inside of `raven_setup_builtins(...)`.
 */
static void raven_builtin(struct raven* raven,
                          const char*   name,
                          builtin_func  builtin) {
  symbol_set_builtin(raven_find_symbol(raven, name), builtin);
}

/*
 * Set up all the builtin functions.
 */
void raven_setup_builtins(struct raven* raven) {
  /*
   * This is the place where all the builtin functions get installed.
   * Essentially, we allocate a symbol and set its builtin pointer
   * to the corresponding function.
   */
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
