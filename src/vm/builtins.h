/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_VM_BUILTINS_H
#define RAVEN_VM_BUILTINS_H

#include "../defs.h"
#include "../core/any.h"

typedef void (*builtin_func)(struct fiber*, any*, unsigned int);

void builtin_throw(struct fiber* fiber, any* arg, unsigned int args);
void builtin_sleep(struct fiber* fiber, any* arg, unsigned int args);
void builtin_fork(struct fiber* fiber, any* arg, unsigned int args);

void builtin_this_connection(struct fiber* fiber, any* arg, unsigned int args);
void builtin_connection_player(struct fiber* fiber, any* arg, unsigned int args);
void builtin_print(struct fiber* fiber, any* arg, unsigned int args);
void builtin_write(struct fiber* fiber, any* arg, unsigned int args);
void builtin_write_to(struct fiber* fiber, any* arg, unsigned int args);
void builtin_input_line(struct fiber* fiber, any* arg, unsigned int args);
void builtin_close(struct fiber* fiber, any* arg, unsigned int args);

void builtin_the(struct fiber* fiber, any* arg, unsigned int args);
void builtin_initializedp(struct fiber* fiber, any* arg, unsigned int args);
void builtin_initialize(struct fiber* fiber, any* arg, unsigned int args);
void builtin_recompile(struct fiber* fiber, any* arg, unsigned int args);

void builtin_arrayp(struct fiber* fiber, any* arg, unsigned int args);
void builtin_stringp(struct fiber* fiber, any* arg, unsigned int args);
void builtin_objectp(struct fiber* fiber, any* arg, unsigned int args);
void builtin_functionp(struct fiber* fiber, any* arg, unsigned int args);

void builtin_nil_proxy(struct fiber* fiber, any* arg, unsigned int args);
void builtin_string_proxy(struct fiber* fiber, any* arg, unsigned int args);
void builtin_array_proxy(struct fiber* fiber, any* arg, unsigned int args);
void builtin_mapping_proxy(struct fiber* fiber, any* arg, unsigned int args);
void builtin_symbol_proxy(struct fiber* fiber, any* arg, unsigned int args);
void builtin_connect_func(struct fiber* fiber, any* arg, unsigned int args);
void builtin_disconnect_func(struct fiber* fiber, any* arg, unsigned int args);

void builtin_clone_object(struct fiber* fiber, any* arg, unsigned int args);

void builtin_object_move(struct fiber* fiber, any* arg, unsigned int args);
void builtin_object_parent(struct fiber* fiber, any* arg, unsigned int args);
void builtin_object_sibling(struct fiber* fiber, any* arg, unsigned int args);
void builtin_object_children(struct fiber* fiber, any* arg, unsigned int args);
void builtin_object_path(struct fiber* fiber, any* arg, unsigned int args);
void builtin_object_set_hb(struct fiber* fiber, any* arg, unsigned int args);
void builtin_object_fst_hb(struct fiber* fiber, any* arg, unsigned int args);
void builtin_object_next_hb(struct fiber* fiber, any* arg, unsigned int args);

void builtin_substr(struct fiber* fiber, any* arg, unsigned int args);
void builtin_mkarray(struct fiber* fiber, any* arg, unsigned int args);
void builtin_append(struct fiber* fiber, any* arg, unsigned int args);

void builtin_keys(struct fiber* fiber, any* arg, unsigned int args);

void builtin_isspace(struct fiber* fiber, any* arg, unsigned int args);
void builtin_wrap(struct fiber* fiber, any* arg, unsigned int args);

void builtin_implements(struct fiber* fiber, any* arg, unsigned int args);
void builtin_call(struct fiber* fiber, any* arg, unsigned int args);

void builtin_this_player(struct fiber* fiber, any* arg, unsigned int args);

void builtin_ls(struct fiber* fiber, any* arg, unsigned int args);
void builtin_resolve(struct fiber* fiber, any* arg, unsigned int args);
void builtin_read_file(struct fiber* fiber, any* arg, unsigned int args);
void builtin_write_file(struct fiber* fiber, any* arg, unsigned int args);
void builtin_cc(struct fiber* fiber, any* arg, unsigned int args);
void builtin_cc_script(struct fiber* fiber, any* arg, unsigned int args);
void builtin_disassemble(struct fiber* fiber, any* arg, unsigned int args);

void builtin_gc(struct fiber* fiber, any* arg, unsigned int args);

void builtin_random(struct fiber* fiber, any* arg, unsigned int args);

#endif
