/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../core/objects/symbol.h"
#include "../core/objects/funcref.h"
#include "../vm/builtins.h"
#include "../vm/fiber.h"
#include "../vm/interpreter.h"
#include "../vm/scheduler.h"

#include "raven.h"


/*
 * Make all of Raven's LPC vars point to `nil`.
 */
static void raven_create_vars(struct raven_vars* vars) {
    vars->nil_proxy       = any_nil();
    vars->string_proxy    = any_nil();
    vars->array_proxy     = any_nil();
    vars->mapping_proxy   = any_nil();
    vars->symbol_proxy    = any_nil();

    vars->connect_func    = NULL;
    vars->disconnect_func = NULL;
}

/*
 * Create a new instance of Raven.
 */
void raven_create(struct raven* raven) {
    object_table_create(raven_objects(raven), raven);
    typeset_create(raven_types(raven), raven);
    log_create(raven_log(raven));
    scheduler_create(raven_scheduler(raven), raven);
    server_create(raven_server(raven), raven);
    filesystem_create(raven_fs(raven), raven);
    raven_setup_builtins(raven);
    raven_create_vars(raven_vars(raven));
    raven->was_interrupted = false;
}

/*
 * Destroy an instance of Raven.
 */
void raven_destroy(struct raven* raven) {
    filesystem_destroy(raven_fs(raven));
    server_destroy(raven_server(raven));
    scheduler_destroy(raven_scheduler(raven));
    log_destroy(raven_log(raven));
    typeset_destroy(raven_types(raven));
    object_table_destroy(raven_objects(raven));
}

/*
 * Print a nice little banner for this program.
 */
void raven_banner(struct raven* raven) {
    log_printf(raven_log(raven), "\n");
    log_printf(raven_log(raven), "        R a v e n\n");
    log_printf(raven_log(raven), "\n");
    log_printf(raven_log(raven), "        The Raven MUD driver\n");
    log_printf(raven_log(raven), "        Version %s\n", RAVEN_VERSION);
    log_printf(raven_log(raven), "\n");
}

/*
 * Load a Raven mudlib, and prepare all important objects.
 */
bool raven_boot(struct raven* raven, const char* mudlib) {
    bool  result;

    result = false;

    /*
     * Print our banner and version info.
     */
    raven_banner(raven);

    /*
     * Set up random number generation.
     */
    srand(time(NULL));

    /*
     * Set up the filesystem anchor, and load all files in the directory.
     */
    filesystem_set_anchor(&raven->fs, mudlib);
    filesystem_load(&raven->fs);

    /*
     * Spawn a new fiber, calling "/secure/master"->main()
     */
    result = raven_call_out(raven, "/secure/master", "main", NULL, 0);

    return result;
}

/*
 * Shut down Raven. This will (in the future) save the state of
 * the system to disk, and gracefully stop all threads.
 */
void raven_shutdown(struct raven* raven) {
    log_printf(raven_log(raven), "Shutting down Raven...\n");
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

    gc_mark_ptr(gc, vars->connect_func);
    gc_mark_ptr(gc, vars->disconnect_func);
}

/*
 * Mark the Raven state during a garbage collection.
 */
void raven_mark(struct gc* gc, struct raven* raven) {
    object_table_mark(gc, raven_objects(raven));
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
 * Assign a port to Raven's server.
 */
void raven_serve_on(struct raven* raven, int port) {
    if (server_serve_on(raven_server(raven), port))
        log_printf(raven_log(raven), "Now serving on port %d...\n", port);
    else
        log_printf(raven_log(raven), "NOT serving on port %d!\n", port);
}

/*
 * Raven's interrupt function.
 */
void raven_interrupt(struct raven* raven) {
    raven->was_interrupted = true;
}

/*
 * Reset Raven's interrupt variable.
 */
void raven_uninterrupt(struct raven* raven) {
    raven->was_interrupted = false;
}

static const char* LOADING_PATTERNS[] = { "|", "/", "-", "\\" };

static void print_loading_pattern() {
    unsigned long size;

    size = sizeof(LOADING_PATTERNS) / sizeof(LOADING_PATTERNS[0]);

    printf("%s\r", LOADING_PATTERNS[time(NULL) % size]);
    fflush(stdout);
}

/*
 * The main loop.
 */
void raven_loop(struct raven* raven) {
    unsigned int     gc_steps;
    raven_timeval_t  tv;

    /*
     * We count the amount of loop iterations until we trigger the
     * next garbage collection. This variable holds the amount of steps.
     */
    gc_steps = 0;

    /*
     * Clear the interrupt flag, so that the loop won't break early.
     */
    raven_uninterrupt(raven);

    /*
     * This is the main loop of the MUD. It consists of a call to the
     * scheduler (which in turn advances all the running game threads),
     * followed by asking the socket server to update the MUD's
     * connections and sessions.
     */
    while (!raven_was_interrupted(raven)) {
        print_loading_pattern();
        if (gc_steps++ % 128 == 0)
            raven_gc(raven);
        scheduler_run(raven_scheduler(raven));

        if (scheduler_is_sleeping(raven_scheduler(raven))) {
            tv.tv_sec  = 0;
            tv.tv_usec = 150000;
        } else {
            tv.tv_sec  = 0;
            tv.tv_usec = 0;
        }

        server_tick(raven_server(raven), tv);
    }
}

/*
 * Run Raven, and then shut down.
 */
void raven_run(struct raven* raven) {
    raven_loop(raven);
    raven_shutdown(raven);
}

/*
 * Look up a symbol by its name inside of Raven's symbol table.
 */
struct symbol* raven_find_symbol(struct raven* raven, const char* name) {
    return object_table_find_symbol(raven_objects(raven), name);
}

/*
 * Generate a new, globally unique symbol.
 */
struct symbol* raven_gensym(struct raven* raven) {
    return object_table_gensym(raven_objects(raven));
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
 * Call a method on an object in a new thread.
 */
bool raven_call_out(struct raven* raven,
                    const  char*  receiver,
                    const  char*  name,
                    any*          args,
                    unsigned int  arg_count) {
    struct object* object;
    struct fiber*  fiber;
    unsigned int   index;

    object = raven_get_object(raven, receiver);
    if (object != NULL) {
        fiber = scheduler_new_fiber(raven_scheduler(raven));
        if (fiber != NULL) {
            fiber_push(fiber, any_from_ptr(object));
            for (index = 0; index < arg_count; index++)
                fiber_push(fiber, args[index]);
            fiber_send(fiber, raven_find_symbol(raven, name), arg_count);
            return true;
        }
    }
    return false;
}

/*
 * Call a funcref in a new thread.
 */
bool raven_call_out_func(struct raven*   raven,
                         struct funcref* funcref,
                         any*            args,
                         unsigned int    arg_count) {
    struct fiber*  fiber;

    fiber = scheduler_new_fiber(raven_scheduler(raven));
    if (fiber != NULL) {
        funcref_enter(funcref, fiber, args, arg_count);
        return true;
    }
    return false;
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
    raven_builtin(raven, "_throw", builtin_throw);
    raven_builtin(raven, "_sleep", builtin_sleep);
    raven_builtin(raven, "fork", builtin_fork);

    raven_builtin(raven, "_this_connection", builtin_this_connection);
    raven_builtin(raven, "_connection_player", builtin_connection_player);
    raven_builtin(raven, "print", builtin_print);
    raven_builtin(raven, "write", builtin_write);
    raven_builtin(raven, "write_to", builtin_write_to);
    raven_builtin(raven, "input_line", builtin_input_line);
    raven_builtin(raven, "_close", builtin_close);

    raven_builtin(raven, "_the", builtin_the);
    raven_builtin(raven, "_initializedp", builtin_initializedp);
    raven_builtin(raven, "_initialize", builtin_initialize);
    raven_builtin(raven, "_recompile", builtin_recompile);

    raven_builtin(raven, "_arrayp", builtin_arrayp);
    raven_builtin(raven, "_stringp", builtin_stringp);
    raven_builtin(raven, "_objectp", builtin_objectp);
    raven_builtin(raven, "_functionp", builtin_functionp);

    raven_builtin(raven, "_nil_proxy", builtin_nil_proxy);
    raven_builtin(raven, "_string_proxy", builtin_string_proxy);
    raven_builtin(raven, "_array_proxy", builtin_array_proxy);
    raven_builtin(raven, "_mapping_proxy", builtin_mapping_proxy);
    raven_builtin(raven, "_symbol_proxy", builtin_symbol_proxy);
    raven_builtin(raven, "_connect_func", builtin_connect_func);
    raven_builtin(raven, "_disconnect_func", builtin_disconnect_func);

    raven_builtin(raven, "clone_object", builtin_clone_object);
    raven_builtin(raven, "_object_move", builtin_object_move);
    raven_builtin(raven, "_object_parent", builtin_object_parent);
    raven_builtin(raven, "_object_sibling", builtin_object_sibling);
    raven_builtin(raven, "_object_children", builtin_object_children);
    raven_builtin(raven, "_object_path", builtin_object_path);
    raven_builtin(raven, "_set_heart_beat", builtin_object_set_hb);
    raven_builtin(raven, "_object_first_heartbeat", builtin_object_fst_hb);
    raven_builtin(raven, "_object_next_heartbeat", builtin_object_next_hb);

    raven_builtin(raven, "_substr", builtin_substr);
    raven_builtin(raven, "mkarray", builtin_mkarray);
    raven_builtin(raven, "_append", builtin_append);
    raven_builtin(raven, "_keys", builtin_keys);

    raven_builtin(raven, "_isspace", builtin_isspace);
    raven_builtin(raven, "_wrap", builtin_wrap);

    raven_builtin(raven, "_implements", builtin_implements);
    raven_builtin(raven, "call", builtin_call);

    raven_builtin(raven, "this_player", builtin_this_player);

    raven_builtin(raven, "_ls", builtin_ls);
    raven_builtin(raven, "_resolve", builtin_resolve);
    raven_builtin(raven, "_cat", builtin_read_file);
    raven_builtin(raven, "_read_file", builtin_read_file);
    raven_builtin(raven, "_write_file", builtin_write_file);
    raven_builtin(raven, "_cc", builtin_cc);
    raven_builtin(raven, "_cc_script", builtin_cc_script);
    raven_builtin(raven, "_disassemble", builtin_disassemble);

    raven_builtin(raven, "_gc", builtin_gc);

    raven_builtin(raven, "_random", builtin_random);
}
