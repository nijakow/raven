/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../runtime/core/blueprint.h"
#include "../runtime/core/objects/object/object.h"
#include "../runtime/core/objects/symbol.h"
#include "../runtime/core/objects/funcref.h"
#include "../runtime/vm/fiber.h"
#include "../runtime/vm/interpreter.h"
#include "../runtime/vm/scheduler.h"

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
 *
 * This will initialize all of Raven's subsystems, and set up
 * a minimal configuration.
 * 
 * However, the mudlib will not be loaded until `raven_boot(...)`
 * is called.
 */
void raven_create(struct raven* raven) {
    object_table_create(raven_objects(raven), raven);
    typeset_create(raven_types(raven), raven);
    log_create(raven_log(raven));
    scheduler_create(raven_scheduler(raven), raven);
    server_create(raven_server(raven), raven);
    fs_create(raven_fs(raven), raven);
    git_repo_create(raven_git(raven));
    users_create(raven_users(raven), raven);
    raven_create_vars(raven_vars(raven));
    raven->was_interrupted = false;

    raven_setup_builtins(raven);
}

/*
 * Destroy an instance of Raven.
 *
 * The subsystems will be destroyed in reverse order of creation.
 * There are exceptions for some subsystems that depend on others.
 */
void raven_destroy(struct raven* raven) {
    git_repo_destroy(raven_git(raven));
    fs_destroy(raven_fs(raven));
    server_destroy(raven_server(raven));
    scheduler_destroy(raven_scheduler(raven));
    log_destroy(raven_log(raven));
    typeset_destroy(raven_types(raven));
    object_table_destroy(raven_objects(raven));
    users_destroy(raven_users(raven));
}

/*
 * Print a nice little banner for this program.
 *
 * This is just a fun little thing to do, and it looks nice.
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
    log_printf(raven_log(raven), "    Copyright (c) Eric Nijakowski 2022+\n");
    log_printf(raven_log(raven), "    Website: %s\n", "https://github.com/nijakow/raven"); /* Using "%s" to keep VSCode from butchering the link :D */
    log_printf(raven_log(raven), "\n");
}

/*
 * Boot Raven with a given mudlib.
 *
 * This will load and validate all of the mudlib's files, and
 * then kick off the master object.
 * 
 * The master object is the only object that will be loaded
 * directly on our request. All other objects will be loaded
 * on-demand by the master object.
 * 
 * The entry point for the master object is "/secure/master".main().
 * 
 * Returns true if the boot was successful, false otherwise.
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
     * Set up the filesystem anchor.
     *
     * TODO: Verify integrity of the mudlib.
     */
    fs_set_anchor(raven_fs(raven), mudlib);

    /*
     * Set up the git repository.
     *
     * This allows us to access the mudlib's git repository,
     * therefore making it possible to use git features such
     * as branches, committing, pushing and pulling from within
     * the running game server.
     */
    git_repo_set_path(raven_git(raven), mudlib);

    /*
     * Spawn a new fiber by calling `"/secure/master".main()`.
     *
     * This will load the master object, and then call its
     * `main()` function.
     */
    result = raven_call_out(raven, "/secure/master", "main", NULL, 0);
    if (!result) {
        /*
         * If we couldn't call out to the master object, then
         * we should probably just issue an error and crash.
         */
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
    fs_mark(gc, raven_fs(raven));
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
    
    return result;
}

/*
 * Raven's interrupt function.
 *
 * This will be called by the interrupt handler, and will
 * cause Raven to break out of its main loop the next time
 * it gets a chance.
 */
void raven_interrupt(struct raven* raven) {
    raven->was_interrupted = true;
}

/*
 * Reset Raven's interrupt variable.
 *
 * This is done when Raven decides to ignore the interrupt
 * and continue running.
 */
void raven_uninterrupt(struct raven* raven) {
    raven->was_interrupted = false;
}

/*
 * Behold the infamous main loop!
 * 
 * Everything that happens in Raven happens here.
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
struct blueprint* raven_get_blueprint(struct raven* raven, const char* path, bool create) {
    return fs_find_blueprint(raven_fs(raven), path, create);
}

/*
 * Resolve an object by its path.
 */
struct object* raven_get_object(struct raven* raven, const char* path, bool create) {
    return fs_find_object(raven_fs(raven), path, create);
}

/*
 * Recompile a blueprint.
 */
bool raven_recompile_with_log(struct raven* raven, const char* path, struct log* log) {
    return fs_recompile_with_log(raven_fs(raven), path, log);
}

/*
 * Recompile a blueprint.
 */
bool raven_recompile(struct raven* raven, const char* path) {
    return raven_recompile_with_log(raven, path, raven_log(raven));
}

/*
 * Recompile an object.
 */
bool raven_recompile_object_with_log(struct raven* raven, struct object* object, struct log* log) {
    struct blueprint* old_bp;
    struct blueprint* new_bp;

    old_bp = object_blueprint(object);

    if (raven_recompile_with_log(raven, blueprint_virt_path(old_bp), log)) {
        new_bp = raven_get_blueprint(raven, blueprint_virt_path(old_bp), false);
        if (new_bp != NULL) {
            object_switch_blueprint(object, new_bp);
            return true;
        }
    }

    return false;
}

/*
 * Recompile an object.
 */
bool raven_recompile_object(struct raven* raven, struct object* object) {
    return raven_recompile_object_with_log(raven, object, raven_log(raven));
}

/*
 * Refresh an object.
 */
bool raven_refresh_object(struct raven* raven, struct object* object) {
    struct blueprint* old_bp;
    struct blueprint* new_bp;
    
    old_bp = object_blueprint(object);
    new_bp = raven_get_blueprint(raven, blueprint_virt_path(old_bp), false);

    if (new_bp != NULL) {
        if (old_bp != new_bp) {
            object_switch_blueprint(object, new_bp);
        }
        return true;
    }

    return false;
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

    object = raven_get_object(raven, receiver, true);
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
