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


/*
 * Start the compilation of a catch block in a try/catch statement.
 *
 * Try/catch statements are a bit tricky, because they implement very
 * unusual execution patterns (i.e. you can "jump" to a catch block from
 * almost anywhere in the code, even from different functions).
 * 
 * Especially in a one-pass compiler like this one, generating multiple
 * execution paths in a linear fashion (and therefore with limited information
 * about the future) is not easy and requires some tricks. The resulting
 * bytecode is not as efficient as it could be, but it is sufficient for
 * our purposes.
 * 
 * The basic idea is that we have a stack of catch labels. A catch label
 * is a label in the bytecode that marks the beginning of a catch block.
 * Every call frame in the VM stack has a slot which contains a reference
 * to the current catch label (or NULL if there is no catch block - in this
 * case, the exception is propagated to the caller one level up on the stack).
 * 
 * When an exception is thrown, the VM looks at the current catch label
 * and jumps to it. If the catch label is NULL, the exception is propagated
 * to the caller. If the catch label is not NULL, the exception is caught
 * and the catch block is executed.
 * 
 * The compiler therefore has to generate instructions that implement the
 * nested nature of try/catch blocks. When a try block is compiled, the
 * current catch label is pushed onto the catch label stack inside of the
 * compiler. The surrounding catch blocks are therefore always available.
 * 
 * As soon as a try block is left (either by leaving the scope normally or
 * when the VM automatically jumps out of the try block and into the catch
 * block), the location inside of the stack frame needs to be updated to
 * point to the surrounding catch block (or NULL if there is none). This
 * is done by the compiler_place_catch() function.
 * 
 * Due to the abovementioned limitations of a one-pass compiler, the
 * compiler_place_catch() function takes a label as an argument. This is
 * due to the fact that the compiler is unable to store the code for the
 * catch block anywhere else other than at the current position in the
 * instruction stream - leaving the try block therefore means jumping
 * to the end of the catch block. The `jumpto` argument is therefore
 * the label that marks the location where the code for the catch block
 * ends.
 */
void compiler_place_catch(struct compiler* compiler, t_compiler_label jumpto) {
    t_compiler_label  catch_label;
    t_compiler_label  next_catch_label;
    bool              has_next_catch;

    /*
     * If there is no catch clause, there is nothing to place.
     * Therefore, we do nothing.
     */
    if (compiler->catch_count == 0) {
        /*
         * Still, we have to jump to the target given to us.
         */
        compiler_jump(compiler, jumpto);
        return;
    }

    /*
     * Pop the label where the current catch block begins.
     */
    catch_label = compiler->catch_labels[--(compiler->catch_count)];

    /*
     * We take the next catch label from the catch stack.
     */
    has_next_catch = compiler_catchlabel(compiler, &next_catch_label);

    /*
     * The try clause is now finished. No exception can happen inside of
     * it anymore, so we update the catch label to point to the surrounding
     * catch clause.
     */
    if (has_next_catch) {
        /*
         * If an error occurs, we jump to the surrounding catch clause.
         */
        codewriter_update_catch(compiler->cw, next_catch_label);
    } else {
        /*
         * No surrounding catch clause, so we clear the catch label.
         */
        codewriter_clear_catch(compiler->cw);
    }

    /*
     * To continuing on the normal code path, we jump to the target given to us.
     */
    compiler_jump(compiler, jumpto);

    /*
     * Now the actual catch block begins!
     * The catch label was previously unplaced, so we place it now.
     * This is where the VM jumps to when an exception is thrown.
     */
    compiler_place_label(compiler, catch_label);
    compiler_close_label(compiler, catch_label);

    /*
     * This is the first instruction in the catch block: To make sure
     * that we do the right thing if yet another exception occurs inside
     * of this catch block, we need to update the catch label to point
     * to the surrounding catch clause.
     * 
     * This is the same as before. The only difference (and the reason for
     * the repetition) is that we are not in the normal code path anymore,
     * but in the catch block.
     */
    if (has_next_catch) {
        /*
         * If an error occurs, we jump to the surrounding catch clause.
         */
        codewriter_update_catch(compiler->cw, next_catch_label);
    } else {
        /*
         * No surrounding catch clause, so we clear the catch label.
         */
        codewriter_clear_catch(compiler->cw);
    }
}
