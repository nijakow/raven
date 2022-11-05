/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_CODEWRITER_H
#define RAVEN_LANG_CODEWRITER_H

#include "../defs.h"
#include "../core/any.h"

#define CW_MAX_BYTECODES (16*1024)
#define CW_MAX_CONSTANTS (1024)
#define CW_MAX_TYPES     (1024)
#define CW_MAX_LABELS    (64)

typedef t_wc t_cw_label;

struct label {
    t_cw_label  loc;
    t_cw_label  target;
};

struct codewriter {
    struct raven*  raven;
    unsigned int   max_locals;
    unsigned int   args;
    bool           varargs;
    size_t         alloc;
    size_t         fill;
    t_bc*          bytecodes;
    size_t         ci;
    size_t         ti;
    t_cw_label     li;
    any            constants[CW_MAX_CONSTANTS];
    struct type*   types[CW_MAX_TYPES];
    struct label   labels[CW_MAX_LABELS];
};

void codewriter_create(struct codewriter* writer, struct raven* raven);
void codewriter_destroy(struct codewriter* writer);

struct function* codewriter_finish(struct codewriter* writer);

void codewriter_report_locals(struct codewriter* writer, unsigned int locals);
void codewriter_report_arg(struct codewriter* writer);
void codewriter_enable_varargs(struct codewriter* writer);

void codewriter_load_self(struct codewriter* writer);
void codewriter_load_const(struct codewriter* writer, any value);
void codewriter_load_array(struct codewriter* writer, t_wc size);
void codewriter_load_mapping(struct codewriter* writer, t_wc size);
void codewriter_load_funcref(struct codewriter* writer, any name);
void codewriter_load_local(struct codewriter* writer, t_wc index);
void codewriter_load_member(struct codewriter* writer, t_wc index);

void codewriter_store_local(struct codewriter* writer, t_wc index);
void codewriter_store_member(struct codewriter* writer, t_wc index);

void codewriter_op(struct codewriter* writer, t_wc op);

void codewriter_push_self(struct codewriter* writer);
void codewriter_push_constant(struct codewriter* writer, any value);
void codewriter_push(struct codewriter* writer);
void codewriter_pop(struct codewriter* writer);

void codewriter_send(struct codewriter* writer, any message, t_bc args);
void codewriter_super_send(struct codewriter* writer, any message, t_bc args);

t_cw_label codewriter_open_label(struct codewriter* writer);
void       codewriter_place_label(struct codewriter* writer, t_cw_label label);
void       codewriter_close_label(struct codewriter* writer, t_cw_label label);
void       codewriter_jump(struct codewriter* writer, t_cw_label label);
void       codewriter_jump_if(struct codewriter* writer, t_cw_label label);
void       codewriter_jump_if_not(struct codewriter* writer, t_cw_label label);
void       codewriter_update_catch(struct codewriter* writer, t_cw_label label);
void       codewriter_clear_catch(struct codewriter* writer);

void       codewriter_return(struct codewriter* writer);

void       codewriter_typeis(struct codewriter* writer, struct type* type);
void       codewriter_typecheck(struct codewriter* writer, struct type* type);
void       codewriter_typecast(struct codewriter* writer, struct type* type);

#endif
