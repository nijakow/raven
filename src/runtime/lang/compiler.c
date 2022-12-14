/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../raven/raven.h"
#include "../core/type.h"

#include "compiler.h"

static void compiler_create_base(struct compiler* compiler) {
    vars_create(&compiler->vars);
    compiler->break_label    = -1;
    compiler->continue_label = -1;
    compiler->catch_count    = 0;
    compiler->mapping_vars   = NULL;
}

void compiler_create(struct compiler*   compiler,
                     struct raven*      raven,
                     struct codewriter* codewriter,
                     struct blueprint*  blueprint) {
    compiler_create_base(compiler);
    compiler->raven        = raven;
    compiler->parent       = NULL;
    compiler->cw           = codewriter;
    compiler->bp           = blueprint;
}

void compiler_create_sub(struct compiler* compiler, struct compiler* parent) {
    compiler_create_base(compiler);
    compiler->raven        = parent->raven;
    compiler->parent       = parent;
    compiler->cw           = parent->cw;
    compiler->bp           = parent->bp;
    compiler->mapping_vars = parent->mapping_vars;
    vars_reparent(&compiler->vars, &parent->vars);
}

void compiler_destroy(struct compiler* compiler) {
    vars_destroy(&compiler->vars);
}

struct function* compiler_finish(struct compiler* compiler) {
    return codewriter_finish(compiler->cw);
}

void compiler_set_mapping_vars(struct compiler* compiler,
                               struct mapping*  vars) {
    compiler->mapping_vars = vars;
}

void compiler_add_arg(struct compiler* compiler,
                      struct type*     type,
                      struct symbol*   name) {
    compiler_add_var(compiler, type, name);
    codewriter_report_arg(compiler->cw);
}

void compiler_add_var(struct compiler* compiler,
                      struct type*     type,
                      struct symbol*   name) {
    struct var_flags flags;

    var_flags_create(&flags);
    vars_add(&compiler->vars, type, name, flags);
    codewriter_report_locals(compiler->cw, vars_count(&compiler->vars));
}

void compiler_enable_varargs(struct compiler* compiler) {
    codewriter_enable_varargs(compiler->cw);
}

bool compiler_load_self(struct compiler* compiler) {
    codewriter_load_self(compiler->cw);
    return true;
}

bool compiler_load_constant(struct compiler* compiler, any value) {
    codewriter_load_const(compiler->cw, value);
    return true;
}

bool compiler_load_array(struct compiler* compiler, unsigned int size) {
    codewriter_load_array(compiler->cw, (t_wc) size);
    return true;
}

bool compiler_load_mapping(struct compiler* compiler, unsigned int size) {
    codewriter_load_mapping(compiler->cw, (t_wc) size);
    return true;
}

bool compiler_load_funcref(struct compiler* compiler, struct symbol* name) {
    codewriter_load_funcref(compiler->cw, any_from_ptr(name));
    return true;
}

bool compiler_load_var_with_type(struct compiler* compiler,
                                 struct symbol*   name,
                                 struct type**    type) {
    unsigned int  index;

    if (vars_find(&compiler->vars, name, type, &index)) {
        codewriter_load_local(compiler->cw, index);
        return true;
    } else if (compiler->bp != NULL
               && vars_find(blueprint_vars(compiler->bp), name, type, &index)) {
        codewriter_load_member(compiler->cw, index);
        return true;
    } else if (compiler->mapping_vars != NULL) {
        if (type != NULL)
            *type = typeset_type_any(raven_types(compiler->raven));
        compiler_push_constant(compiler, any_from_ptr(compiler->mapping_vars));
        compiler_load_constant(compiler, any_from_ptr(name));
        compiler_op(compiler, RAVEN_OP_INDEX);
        return true;
    } else {
        return false;
    }
}

bool compiler_load_var(struct compiler* compiler, struct symbol* name) {
    return compiler_load_var_with_type(compiler, name, NULL);
}

bool compiler_store_var_with_type(struct compiler* compiler,
                                  struct symbol*   name,
                                  struct type**    type) {
    struct type*  our_type;
    unsigned int  index;

    if (vars_find(&compiler->vars, name, &our_type, &index)) {
        if (type != NULL) *type = our_type;
        if (our_type != NULL)
            compiler_typecheck(compiler, our_type);
        codewriter_store_local(compiler->cw, index);
        return true;
    } else if (compiler->bp != NULL
               && vars_find(blueprint_vars(compiler->bp), name, &our_type, &index)) {
        if (type != NULL) *type = our_type;
        if (our_type != NULL)
            compiler_typecheck(compiler, our_type);
        codewriter_store_member(compiler->cw, index);
        return true;
    } else if (compiler->mapping_vars != NULL) {
        if (type != NULL)
            *type = typeset_type_any(raven_types(compiler->raven));
        compiler_push_constant(compiler, any_from_ptr(compiler->mapping_vars));
        compiler_push_constant(compiler, any_from_ptr(name));
        compiler_op(compiler, RAVEN_OP_INDEX_ASSIGN);
        return true;
    } else {
        return false;
    }
}

bool compiler_store_var(struct compiler* compiler, struct symbol* name) {
    return compiler_store_var_with_type(compiler, name, NULL);
}

void compiler_push_self(struct compiler* compiler) {
    codewriter_push_self(compiler->cw);
}

void compiler_push_constant(struct compiler* compiler, any value) {
    codewriter_push_constant(compiler->cw, value);
}

void compiler_push(struct compiler* compiler) {
    codewriter_push(compiler->cw);
}

void compiler_pop(struct compiler* compiler) {
    codewriter_pop(compiler->cw);
}

void compiler_op(struct compiler* compiler, enum raven_op op) {
    codewriter_op(compiler->cw, (t_wc) op);
}

void compiler_call_builtin(struct compiler* compiler,
                           struct symbol*   message,
                           unsigned int     arg_count) {
    codewriter_call_builtin(compiler->cw, any_from_ptr(message), (t_wc) arg_count);
}

void compiler_send(struct compiler* compiler,
                   struct symbol*   message,
                   unsigned int     arg_count) {
    codewriter_send(compiler->cw, any_from_ptr(message), (t_wc) arg_count);
}

void compiler_super_send(struct compiler* compiler,
                         struct symbol*   message,
                         unsigned int     arg_count) {
    codewriter_super_send(compiler->cw, any_from_ptr(message), (t_wc) arg_count);
}

void compiler_return(struct compiler* compiler) {
    codewriter_return(compiler->cw);
}

void compiler_typeis(struct compiler* compiler, struct type* type) {
    codewriter_typeis(compiler->cw, type);
}

void compiler_typecheck(struct compiler* compiler, struct type* type) {
    codewriter_typecheck(compiler->cw, type);
}

void compiler_typecast(struct compiler* compiler, struct type* type) {
    codewriter_typecast(compiler->cw, type);
}

t_compiler_label compiler_open_label(struct compiler* compiler) {
    return codewriter_open_label(compiler->cw);
}

t_compiler_label compiler_open_break_label(struct compiler* compiler) {
    compiler->break_label = compiler_open_label(compiler);
    return compiler->break_label;
}

t_compiler_label compiler_open_continue_label(struct compiler* compiler) {
    compiler->continue_label = compiler_open_label(compiler);
    return compiler->continue_label;
}

void compiler_place_label(struct compiler* compiler, t_compiler_label label) {
    codewriter_place_label(compiler->cw, label);
}

void compiler_close_label(struct compiler* compiler, t_compiler_label label) {
    codewriter_close_label(compiler->cw, label);
}

void compiler_jump(struct compiler* compiler, t_compiler_label label) {
    codewriter_jump(compiler->cw, label);
}

void compiler_jump_if(struct compiler* compiler, t_compiler_label label) {
    codewriter_jump_if(compiler->cw, label);
}

void compiler_jump_if_not(struct compiler* compiler, t_compiler_label label) {
    codewriter_jump_if_not(compiler->cw, label);
}

void compiler_break(struct compiler* compiler) {
    struct compiler*  c;

    for (c = compiler; c != NULL; c = c->parent) {
        if (c->break_label >= 0) {
            compiler_jump(compiler, c->break_label);
            break;
        }
    }
}

void compiler_continue(struct compiler* compiler) {
    struct compiler*  c;

    for (c = compiler; c != NULL; c = c->parent) {
        if (c->continue_label >= 0) {
            compiler_jump(compiler, c->continue_label);
            break;
        }
    }
}

static inline bool compiler_catchlabel(struct compiler*  compiler,
                                       t_compiler_label* label_loc) {
    if (compiler == NULL)
        return false;
    else if (compiler->catch_count == 0)
        return compiler_catchlabel(compiler->parent, label_loc);
    else {
        if (label_loc != NULL)
            *label_loc = compiler->catch_labels[compiler->catch_count - 1];
        return true;
    }
}

bool compiler_open_catch(struct compiler* compiler) {
    t_compiler_label  label;

    if (compiler->catch_count >= COMPILER_MAX_CATCH)
        return false;

    label = compiler_open_label(compiler);
    compiler->catch_labels[compiler->catch_count++] = label;
    codewriter_update_catch(compiler->cw, label);

    return true;
}

void compiler_place_catch(struct compiler* compiler) {
    t_compiler_label  label;

    if (compiler->catch_count == 0)
        return;

    label = compiler->catch_labels[--(compiler->catch_count)];

    compiler_place_label(compiler, label);
    compiler_close_label(compiler, label);
    /*
     * We pop the last catch label from the catch stack.
     */
    if (compiler_catchlabel(compiler, &label)) {
        /*
         * If an error occurs, we jump to the surrounding catch clause.
         */
        codewriter_update_catch(compiler->cw, label);
    } else {
        /*
         * No surrounding catch clause!
         */
        codewriter_clear_catch(compiler->cw);
    }
}
