/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */


/*
 * Builtin functions are LPCs way to call into the C code of the
 * driver. They are implemented in the runtime/vm/builtins.c file.
 * 
 * This file contains the link between the builtin functions and the
 * symbols that represent them.
 * 
 * When Raven starts up, it calls `raven_setup_builtins(...)` to
 * install all the builtin functions. For each builtin, we place
 * a pointer to the corresponding function in the corresponding
 * symbol object.
 */

#include "../runtime/core/objects/symbol.h"
#include "../runtime/vm/builtins.h"

#include "raven.h"

/*
 * Declare a new builtin.
 * Used only inside of `raven_setup_builtins(...)`.
 */
static void raven_builtin(struct raven* raven,
                          const char*   name,
                          builtin_func  builtin) {
    /*
     * Find the symbol object for the given name,
     * and set its builtin pointer to the given function.
     */
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
    raven_builtin(raven, "$read_byte_from", builtin_read_byte_from);
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

    raven_builtin(raven, "$object_move", builtin_object_move);
    raven_builtin(raven, "$object_parent", builtin_object_parent);
    raven_builtin(raven, "$object_sibling", builtin_object_sibling);
    raven_builtin(raven, "$object_children", builtin_object_children);
    raven_builtin(raven, "$object_path", builtin_object_path);
    raven_builtin(raven, "$activate_heart_beat", builtin_object_set_hb);
    raven_builtin(raven, "$deactivate_heart_beat", builtin_object_unset_hb);
    raven_builtin(raven, "$object_first_heartbeat", builtin_object_fst_hb);
    raven_builtin(raven, "$object_next_heartbeat", builtin_object_next_hb);

    raven_builtin(raven, "$loaded", builtin_loaded);
    raven_builtin(raven, "$outdated", builtin_outdated);

    raven_builtin(raven, "$substr", builtin_substr);
    raven_builtin(raven, "$mkarray", builtin_mkarray);
    raven_builtin(raven, "$append", builtin_append);
    raven_builtin(raven, "$insert", builtin_insert);
    raven_builtin(raven, "$remove", builtin_remove);
    raven_builtin(raven, "$keys", builtin_keys);

    raven_builtin(raven, "$isspace", builtin_isspace);
    raven_builtin(raven, "$wrap", builtin_wrap);

    raven_builtin(raven, "$implements", builtin_implements);
    raven_builtin(raven, "$call", builtin_call);

    raven_builtin(raven, "$this_player", builtin_this_player);

    raven_builtin(raven, "$ls", builtin_ls);
    raven_builtin(raven, "$resolve", builtin_resolve);
    raven_builtin(raven, "$cat", builtin_read_file);
    raven_builtin(raven, "$file_exists", builtin_file_exists);
    raven_builtin(raven, "$file_is_directory", builtin_file_is_directory);
    raven_builtin(raven, "$read_file", builtin_read_file);
    raven_builtin(raven, "$write_file", builtin_write_file);
    raven_builtin(raven, "$rm", builtin_rm);
    raven_builtin(raven, "$cc", builtin_cc);
    raven_builtin(raven, "$cc_script", builtin_cc_script);
    raven_builtin(raven, "$disassemble", builtin_disassemble);

    raven_builtin(raven, "$gc", builtin_gc);

    raven_builtin(raven, "$random", builtin_random);

    raven_builtin(raven, "$git_reset_hard", builtin_git_reset_hard);
    raven_builtin(raven, "$git_stage_all", builtin_git_stage_all);
    raven_builtin(raven, "$git_commit", builtin_git_commit);
    raven_builtin(raven, "$git_push", builtin_git_push);
    raven_builtin(raven, "$git_pull", builtin_git_pull);
    raven_builtin(raven, "$git_checkout_branch", builtin_git_checkout_branch);

    raven_builtin(raven, "$login", builtin_login);
}
