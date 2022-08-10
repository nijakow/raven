/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "compiler.h"

static void compiler_create_base(struct compiler* compiler) {
  vars_create(&compiler->vars);
  compiler->break_label    = -1;
  compiler->continue_label = -1;
}

void compiler_create(struct compiler*   compiler,
                     struct codewriter* codewriter,
                     struct blueprint*  blueprint) {
  compiler_create_base(compiler);
  compiler->parent = NULL;
  compiler->cw     = codewriter;
  compiler->bp     = blueprint;
}

void compiler_create_sub(struct compiler* compiler, struct compiler* parent) {
  compiler_create_base(compiler);
  compiler->parent = parent;
  compiler->cw     = parent->cw;
  compiler->bp     = parent->bp;
  vars_reparent(&compiler->vars, &parent->vars);
}

void compiler_destroy(struct compiler* compiler) {
  vars_destroy(&compiler->vars);
}

struct function* compiler_finish(struct compiler* compiler) {
  return codewriter_finish(compiler->cw);
}

void compiler_add_arg(struct compiler* compiler,
                      struct type*     type,
                      struct symbol*   name) {
  /* TODO: Increase arg count */
  compiler_add_var(compiler, type, name);
}

void compiler_add_var(struct compiler* compiler,
                      struct type*     type,
                      struct symbol*   name) {
  vars_add(&compiler->vars, type, name);
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
  } else if (vars_find(blueprint_vars(compiler->bp), name, type, &index)) {
    codewriter_load_member(compiler->cw, index);
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
  unsigned int  index;

  if (vars_find(&compiler->vars, name, type, &index)) {
    codewriter_store_local(compiler->cw, index);
    return true;
  } else if (vars_find(blueprint_vars(compiler->bp), name, type, &index)) {
    codewriter_store_member(compiler->cw, index);
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

void compiler_push(struct compiler* compiler) {
  codewriter_push(compiler->cw);
}

void compiler_pop(struct compiler* compiler) {
  codewriter_pop(compiler->cw);
}

void compiler_op(struct compiler* compiler, enum raven_op op) {
  codewriter_op(compiler->cw, (t_wc) op);
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

void compiler_cast(struct compiler* compiler, struct type* type) {
  codewriter_cast(compiler->cw, type);
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
