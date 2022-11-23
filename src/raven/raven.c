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
    log_printf(raven_log(raven), "        8b,dPPYba, ,adPPYYba, 8b       d8  ,adPPYba, 8b,dPPYba,\n");
    log_printf(raven_log(raven), "        88P'   \"Y8 \"\"     `Y8 `8b     d8' a8P_____88 88P'   `\"8a\n");
    log_printf(raven_log(raven), "        88         ,adPPPPP88  `8b   d8'  8PP\"\"\"\"\"\"\" 88       88\n");
    log_printf(raven_log(raven), "        88         88,    ,88   `8b,d8'   \"8b,   ,aa 88       88\n");
    log_printf(raven_log(raven), "        88         `\"8bbdP\"Y8     \"8\"      `\"Ybbd8\"' 88       88\n");
    log_printf(raven_log(raven), "\n");
    log_printf(raven_log(raven), "    The Raven MUD driver\n");
    log_printf(raven_log(raven), "    Version %s\n", RAVEN_VERSION);
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
    if (!filesystem_load(&raven->fs)) {
        log_printf(raven_log(raven), "Could not load mudlib: %s.\n", strerror(errno));
    }

    /*
     * Spawn a new fiber, calling "/secure/master"->main()
     */
    result = raven_call_out(raven, "/secure/master", "main", NULL, 0);
    if (!result) {
        log_printf(raven_log(raven), "Could not call out to \"/secure/master\".main()!\n");
    }

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
bool raven_serve_on(struct raven* raven, int port) {
    bool  result;

    result = server_serve_on(raven_server(raven), port);

    if (result)
        log_printf(raven_log(raven), "Now serving on port %d...\n", port);
    else
        log_printf(raven_log(raven), "NOT serving on port %d!\n", port);
    
    return true;
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
    raven_builtin(raven, "$open_port", builtin_open_port);

    raven_builtin(raven, "$throw", builtin_throw);
    raven_builtin(raven, "$sleep", builtin_sleep);
    raven_builtin(raven, "$fork", builtin_fork);

    raven_builtin(raven, "$this_connection", builtin_this_connection);
    raven_builtin(raven, "$connection_player", builtin_connection_player);
    raven_builtin(raven, "$this_locals", builtin_this_locals);
    raven_builtin(raven, "$print", builtin_print);
    raven_builtin(raven, "$write_byte_to", builtin_write_byte_to);
    raven_builtin(raven, "$input_line", builtin_input_line);
    raven_builtin(raven, "$close", builtin_close);

    raven_builtin(raven, "$the", builtin_the);
    raven_builtin(raven, "$initializedp", builtin_initializedp);
    raven_builtin(raven, "$initialize", builtin_initialize);
    raven_builtin(raven, "$recompile", builtin_recompile);

    raven_builtin(raven, "$arrayp", builtin_arrayp);
    raven_builtin(raven, "$stringp", builtin_stringp);
    raven_builtin(raven, "$objectp", builtin_objectp);
    raven_builtin(raven, "$functionp", builtin_functionp);

    raven_builtin(raven, "$nil_proxy", builtin_nil_proxy);
    raven_builtin(raven, "$string_proxy", builtin_string_proxy);
    raven_builtin(raven, "$array_proxy", builtin_array_proxy);
    raven_builtin(raven, "$mapping_proxy", builtin_mapping_proxy);
    raven_builtin(raven, "$symbol_proxy", builtin_symbol_proxy);
    raven_builtin(raven, "$connect_func", builtin_connect_func);
    raven_builtin(raven, "$disconnect_func", builtin_disconnect_func);

    raven_builtin(raven, "$clone_object", builtin_clone_object);
    raven_builtin(raven, "$object_move", builtin_object_move);
    raven_builtin(raven, "$object_parent", builtin_object_parent);
    raven_builtin(raven, "$object_sibling", builtin_object_sibling);
    raven_builtin(raven, "$object_children", builtin_object_children);
    raven_builtin(raven, "$object_path", builtin_object_path);
    raven_builtin(raven, "$set_heart_beat", builtin_object_set_hb);
    raven_builtin(raven, "$object_first_heartbeat", builtin_object_fst_hb);
    raven_builtin(raven, "$object_next_heartbeat", builtin_object_next_hb);

    raven_builtin(raven, "$substr", builtin_substr);
    raven_builtin(raven, "$mkarray", builtin_mkarray);
    raven_builtin(raven, "$append", builtin_append);
    raven_builtin(raven, "$keys", builtin_keys);

    raven_builtin(raven, "$isspace", builtin_isspace);
    raven_builtin(raven, "$wrap", builtin_wrap);

    raven_builtin(raven, "$implements", builtin_implements);
    raven_builtin(raven, "$call", builtin_call);

    raven_builtin(raven, "$this_player", builtin_this_player);

    raven_builtin(raven, "$ls", builtin_ls);
    raven_builtin(raven, "$resolve", builtin_resolve);
    raven_builtin(raven, "$cat", builtin_read_file);
    raven_builtin(raven, "$file_is_directory", builtin_file_is_directory);
    raven_builtin(raven, "$read_file", builtin_read_file);
    raven_builtin(raven, "$write_file", builtin_write_file);
    raven_builtin(raven, "$cc", builtin_cc);
    raven_builtin(raven, "$cc_script", builtin_cc_script);
    raven_builtin(raven, "$disassemble", builtin_disassemble);

    raven_builtin(raven, "$gc", builtin_gc);

    raven_builtin(raven, "$random", builtin_random);
}
