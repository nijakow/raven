/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

#include "bytecodes.h"
#include "codewriter.h"
#include "compiler.h"

#include "parsepiler.h"


void parser_error(struct parser* parser, const char* format, ...) {
  va_list args;

  va_start(args, format);
  log_vprintf(parser_log(parser), format, args);
  va_end(args);
}

bool parsepile_expect(struct parser* parser, enum token_type type) {
  if (parser_check(parser, type))
    return true;
  else {
    parser_error(parser, "Syntax error, expected %s\n", token_type_name(type));
    return false;
  }
}

bool parsepile_expect_noadvance(struct parser* parser, enum token_type type) {
  if (parser_is(parser, type))
    return true;
  else {
    parser_error(parser, "Syntax error, expected %s\n", token_type_name(type));
    return false;
  }
}

bool parse_symbol(struct parser* parser, struct symbol** loc) {
  if (!parser_is(parser, TOKEN_TYPE_IDENT))
    return false;
  else {
    *loc = parser_as_symbol(parser);
    parser_advance(parser);
    return *loc != NULL;
  }
}

bool parse_type(struct parser* parser, struct type** loc) {
  *loc = NULL;
  if (parser_check(parser, TOKEN_TYPE_KW_VOID))
    *loc = typeset_type_void(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_ANY))
    *loc = typeset_type_any(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_BOOL))
    *loc = typeset_type_bool(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_INT))
    *loc = typeset_type_int(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_CHAR))
    *loc = typeset_type_char(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_STRING))
    *loc = typeset_type_string(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_OBJECT))
    *loc = typeset_type_object(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_FUNCTION))
    *loc = typeset_type_funcref(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_MAPPING))
    *loc = typeset_type_mapping(raven_types(parser_raven(parser)));
  else
    return false;
  /* Parse asterisks */
  while (parser_check(parser, TOKEN_TYPE_STAR)) {
    /*
     * For now, we don't have array types yet, so we default
     * to `any`.
     */
    *loc = typeset_type_any(raven_types(parser_raven(parser)));
  }
  return true;
}

bool parse_type_and_name(struct parser*  parser,
                         struct type**   type,
                         struct symbol** name) {
  return parse_type(parser, type) && parse_symbol(parser, name);
}

bool parsepile_load_var(struct parser*   parser,
                         struct compiler* compiler,
                         struct symbol*   name) {
  struct type*  type;
  bool          result;

  result = compiler_load_var_with_type(compiler, name, &type);
  if (result) parser_set_exprtype(parser, type);
  return true;
}

bool parsepile_store_var(struct parser*   parser,
                         struct compiler* compiler,
                         struct symbol*   name) {
  struct type*  type;
  bool          result;

  result = compiler_store_var_with_type(compiler, name, &type);
  if (result && !type_match(type, parser_get_exprtype(parser))) {
    parser_error(parser, "Warning: possible type mismatch!\n");
  }
  return true;
}

bool parsepile_return_with_typecheck(struct parser*   parser,
                                     struct compiler* compiler) {
  struct type*  return_type;
  struct type*  expr_type;

  return_type = parser_get_returntype(parser);
  expr_type   = parser_get_exprtype(parser);

  if (return_type != NULL) {
    if (!type_match(return_type, expr_type)) {
      parser_error(parser, "Warning: possible return type mismatch!\n");
    }
    compiler_typecast(compiler, return_type);
  }

  compiler_return(compiler);

  return true;
}

bool parsepile_expr(struct parser* parser, struct compiler* compiler, int pr);
bool parsepile_expression(struct parser* parser, struct compiler* compiler);
bool parsepile_instruction(struct parser* parser, struct compiler* compiler);

bool parsepile_args(struct parser*    parser,
                    struct compiler*  compiler,
                    unsigned int*     arg_count,
                    enum token_type   terminator) {
  bool  result;

  result = false;
  *arg_count = 0;
  if (parser_check(parser, terminator))
    result = true;
  else {
    while (true) {
      if (!parsepile_expression(parser, compiler))
        break;
      compiler_push(compiler);
      (*arg_count)++;
      if (parser_check(parser, terminator)) {
        result = true;
        break;
      }
      if (!parsepile_expect(parser, TOKEN_TYPE_COMMA))
        break;
    }
  }

  return result;
}

bool parsepile_array(struct parser* parser, struct compiler* compiler) {
  unsigned int  arg_count;

  if (parsepile_args(parser, compiler, &arg_count, TOKEN_TYPE_RCURLY)) {
    compiler_load_array(compiler, arg_count);
    parser_set_exprtype_to_array(parser); /* TODO: Array type? */
    return true;
  }
  return false;
}

bool parsepile_mapping(struct parser* parser, struct compiler* compiler) {
  unsigned int  arg_count;

  if (parsepile_args(parser, compiler, &arg_count, TOKEN_TYPE_RBRACK)) {
    compiler_load_mapping(compiler, arg_count);
    parser_set_exprtype_to_mapping(parser); /* TODO: Mapping type? */
    return true;
  }
  return false;
}

bool parsepile_simple_expr(struct parser*   parser,
                           struct compiler* compiler,
                           int              pr) {
  struct symbol*  symbol;
  struct type*    type;
  unsigned int    argcount;
  bool            result;

  result = false;
  if (parse_symbol(parser, &symbol)) {
    if (parser_check(parser, TOKEN_TYPE_LPAREN)) {
      compiler_push_self(compiler);
      result = parsepile_args(parser, compiler, &argcount, TOKEN_TYPE_RPAREN);
      compiler_send(compiler, symbol, argcount);
      parser_set_exprtype_to_any(parser);
    } else if (parser_check(parser, TOKEN_TYPE_ASSIGNMENT)) {
      if (parsepile_expr(parser, compiler, pr)) {
        result = parsepile_store_var(parser, compiler, symbol);
      }
      /* TODO: Do a typecheck */
    } else if (parser_check(parser, TOKEN_TYPE_INC)) {
      result = compiler_load_var(compiler, symbol);
      compiler_push(compiler);
      compiler_push(compiler);
      compiler_load_constant(compiler, any_from_int(1));
      compiler_op(compiler, RAVEN_OP_ADD);
      result = result && compiler_store_var(compiler, symbol);
      compiler_pop(compiler);
      parser_set_exprtype_to_any(parser); /* TODO: Check and infer */
    } else if (parser_check(parser, TOKEN_TYPE_DEC)) {
      result = compiler_load_var(compiler, symbol);
      compiler_push(compiler);
      compiler_push(compiler);
      compiler_load_constant(compiler, any_from_int(1));
      compiler_op(compiler, RAVEN_OP_SUB);
      result = result && compiler_store_var(compiler, symbol);
      compiler_pop(compiler);
    } else {
      result = parsepile_load_var(parser, compiler, symbol);
    }
    parser_set_exprtype_to_any(parser); /* TODO: Check and infer */
  } else if (parser_check(parser, TOKEN_TYPE_KW_NEW)) {
    if (parsepile_expect(parser, TOKEN_TYPE_LPAREN)) {
      parsepile_expression(parser, compiler);
      compiler_op(compiler, RAVEN_OP_NEW);
      compiler_push(compiler);
      compiler_push(compiler);
      if (parser_check(parser, TOKEN_TYPE_RPAREN))
        argcount = 0;
      else {
        if (!parsepile_expect(parser, TOKEN_TYPE_COMMA))
          return false;
        else {
          parsepile_args(parser, compiler, &argcount, TOKEN_TYPE_RPAREN);
        }
      }
      compiler_send(compiler,
                    raven_find_symbol(parser->raven, "create"),
                    argcount);
      compiler_pop(compiler);
      result = true;
    }
    parser_set_exprtype_to_object(parser);
  } else if (parser_check(parser, TOKEN_TYPE_SCOPE)) {
    if (parse_symbol(parser, &symbol)
     && parsepile_expect(parser, TOKEN_TYPE_LPAREN)) {
      compiler_push_self(compiler);
      result = parsepile_args(parser, compiler, &argcount, TOKEN_TYPE_RPAREN);
      compiler_super_send(compiler, symbol, argcount);
    }
    parser_set_exprtype_to_any(parser); /* TODO: Check and infer */
  } else if (parser_check(parser, TOKEN_TYPE_STAR)) {
    result = parsepile_expr(parser, compiler, 1);
    compiler_op(compiler, RAVEN_OP_DEREF);
    parser_set_exprtype_to_any(parser); /* TODO: Check and infer */
  } else if (parser_check(parser, TOKEN_TYPE_LPAREN)) {
    if (parse_type(parser, &type)) {
      result = parsepile_expect(parser, TOKEN_TYPE_RPAREN)
            && parsepile_expr(parser, compiler, pr);
      compiler_typecast(compiler, type);
      parser_set_exprtype(parser, type);
    } else {
      result = parsepile_expression(parser, compiler)
            && parsepile_expect(parser, TOKEN_TYPE_RPAREN);
    }
  } else if (parser_check(parser, TOKEN_TYPE_KW_THIS)) {
    compiler_load_self(compiler);
    result = true;
    parser_set_exprtype_to_any(parser);
  } else if (parser_check(parser, TOKEN_TYPE_KW_NIL)) {
    compiler_load_constant(compiler, any_nil());
    result = true;
    parser_set_exprtype_to_any(parser);
  } else if (parser_check(parser, TOKEN_TYPE_KW_TRUE)) {
    compiler_load_constant(compiler, any_from_int(1));
    result = true;
    parser_set_exprtype_to_bool(parser);
  } else if (parser_check(parser, TOKEN_TYPE_KW_FALSE)) {
    compiler_load_constant(compiler, any_from_int(0));
    result = true;
    parser_set_exprtype_to_bool(parser);
  } else if (parser_is(parser, TOKEN_TYPE_CHAR)) {
    compiler_load_constant(compiler, any_from_char(parser_as_char(parser)));
    parser_advance(parser);
    result = true;
    parser_set_exprtype_to_char(parser);
  } else if (parser_is(parser, TOKEN_TYPE_INT)) {
    compiler_load_constant(compiler, any_from_int(parser_as_int(parser)));
    parser_advance(parser);
    result = true;
    parser_set_exprtype_to_int(parser);
  } else if (parser_is(parser, TOKEN_TYPE_STRING)) {
    compiler_load_constant(compiler, any_from_ptr(parser_as_string(parser)));
    parser_advance(parser);
    result = true;
    parser_set_exprtype_to_string(parser);
  } else if (parser_check(parser, TOKEN_TYPE_LCURLY)) {
    result = parsepile_array(parser, compiler);
    parser_set_exprtype_to_array(parser);
  } else if (parser_check(parser, TOKEN_TYPE_LBRACK)) {
    result = parsepile_mapping(parser, compiler);
    parser_set_exprtype_to_mapping(parser);
  }
  return result;
}

bool parsepile_or(struct parser* parser, struct compiler* compiler) {
  t_compiler_label  label;
  bool              result;

  label = compiler_open_label(compiler);
  compiler_jump_if(compiler, label);
  result = parsepile_expr(parser, compiler, 11);
  compiler_place_label(compiler, label);
  compiler_close_label(compiler, label);
  parser_set_exprtype_to_any(parser); /* TODO: Infer */
  return result;
}

bool parsepile_and(struct parser* parser, struct compiler* compiler) {
  t_compiler_label  label;
  bool              result;

  label = compiler_open_label(compiler);
  compiler_jump_if_not(compiler, label);
  result = parsepile_expr(parser, compiler, 10);
  compiler_place_label(compiler, label);
  compiler_close_label(compiler, label);
  parser_set_exprtype_to_any(parser); /* TODO: Infer */
  return result;
}

bool parsepile_ternary(struct parser* parser, struct compiler* compiler) {
  t_compiler_label  false_part;
  t_compiler_label  end;
  bool              result;

  false_part = compiler_open_label(compiler);
  end        = compiler_open_label(compiler);
  result     = false;

  compiler_jump_if_not(compiler, false_part);
  if (parsepile_expr(parser, compiler, 12)
   && parsepile_expect(parser, TOKEN_TYPE_COLON)) {
    compiler_jump(compiler, end);
    compiler_place_label(compiler, false_part);
    result = parsepile_expr(parser, compiler, 12);
  }

  compiler_place_label(compiler, end);

  compiler_close_label(compiler, false_part);
  compiler_close_label(compiler, end);

  parser_set_exprtype_to_any(parser); /* TODO: Infer */

  return result;
}

bool parsepile_op(struct parser*   parser,
                  struct compiler* compiler,
                  int              pr,
                  bool*            should_continue) {
  struct symbol*  symbol;
  unsigned int    args;
  bool            result;

  *should_continue = true;
   result          = true;

  if (pr >= 1 && parser_check(parser, TOKEN_TYPE_ARROW)) {
    result = false;
    compiler_push(compiler);
    if (parse_symbol(parser, &symbol)
     && parsepile_expect(parser, TOKEN_TYPE_LPAREN)) {
      result = parsepile_args(parser, compiler, &args, TOKEN_TYPE_RPAREN);
      compiler_send(compiler, symbol, args);
    }
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (parser_check(parser, TOKEN_TYPE_LBRACK)) {
    compiler_push(compiler);
    parsepile_expression(parser, compiler);
    if (!parsepile_expect(parser, TOKEN_TYPE_RBRACK))
      return false;
    if (parser_check(parser, TOKEN_TYPE_ASSIGNMENT)) {
      compiler_push(compiler);
      parsepile_expr(parser, compiler, pr);
      compiler_op(compiler, RAVEN_OP_INDEX_ASSIGN);
    } else {
      compiler_op(compiler, RAVEN_OP_INDEX);
    }
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 13 && parser_check(parser, TOKEN_TYPE_QUESTION)) {
    result = parsepile_ternary(parser, compiler);
  } else if (pr >= 12 && parser_check(parser, TOKEN_TYPE_OR)) {
    result = parsepile_or(parser, compiler);
  } else if (pr >= 11 && parser_check(parser, TOKEN_TYPE_AND)) {
    result = parsepile_and(parser, compiler);
  } else if (pr >= 7 && parser_check(parser, TOKEN_TYPE_EQUALS)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 6);
    compiler_op(compiler, RAVEN_OP_EQ);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 7 && parser_check(parser, TOKEN_TYPE_NOT_EQUALS)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 6);
    compiler_op(compiler, RAVEN_OP_INEQ);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 6 && parser_check(parser, TOKEN_TYPE_LESS)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 6);
    compiler_op(compiler, RAVEN_OP_LESS);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 6 && parser_check(parser, TOKEN_TYPE_LEQ)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 6);
    compiler_op(compiler, RAVEN_OP_LEQ);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 6 && parser_check(parser, TOKEN_TYPE_GREATER)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 6);
    compiler_op(compiler, RAVEN_OP_GREATER);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 6 && parser_check(parser, TOKEN_TYPE_GEQ)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 6);
    compiler_op(compiler, RAVEN_OP_GEQ);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 4 && parser_check(parser, TOKEN_TYPE_PLUS)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 3);
    compiler_op(compiler, RAVEN_OP_ADD);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 4 && parser_check(parser, TOKEN_TYPE_MINUS)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 3);
    compiler_op(compiler, RAVEN_OP_SUB);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 3 && parser_check(parser, TOKEN_TYPE_STAR)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 2);
    compiler_op(compiler, RAVEN_OP_MUL);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 3 && parser_check(parser, TOKEN_TYPE_SLASH)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 2);
    compiler_op(compiler, RAVEN_OP_DIV);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else if (pr >= 3 && parser_check(parser, TOKEN_TYPE_PERCENT)) {
    compiler_push(compiler);
    result = parsepile_expr(parser, compiler, 2);
    compiler_op(compiler, RAVEN_OP_MOD);
    parser_set_exprtype_to_any(parser); /* TODO: Infer */
  } else {
    *should_continue = false;
  }

  return result;
}

bool parsepile_expr(struct parser* parser, struct compiler* compiler, int pr) {
  bool            should_continue;
  struct symbol*  symbol;

  if (parser_check(parser, TOKEN_TYPE_AMPERSAND)) {
    if (!parse_symbol(parser, &symbol))
      return false;
    compiler_load_funcref(compiler, symbol);
    parser_set_exprtype_to_any(parser); /* TODO: Function type */
  } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_PLUS)) {
    return parsepile_expr(parser, compiler, 1);
  } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_MINUS)) {
    if (!parsepile_expr(parser, compiler, 1))
      return false;
    compiler_op(compiler, RAVEN_OP_NEGATE);
  } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_NOT)) {
    if (!parsepile_expr(parser, compiler, 1))
      return false;
    compiler_op(compiler, RAVEN_OP_NOT);
  } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_KW_SIZEOF)) {
    if (!parsepile_expr(parser, compiler, 1))
      return false;
    compiler_op(compiler, RAVEN_OP_SIZEOF);
  } else if (!parsepile_simple_expr(parser, compiler, pr)) {
    return false;
  }

  should_continue = true;
  while (should_continue) {
    if (!parsepile_op(parser, compiler, pr, &should_continue))
      return false;
  }

  return true;
}

bool parsepile_expression(struct parser* parser, struct compiler* compiler) {
  return parsepile_expr(parser, compiler, 100);
}

bool parsepile_parenthesized_expression(struct parser*   parser,
                                        struct compiler* compiler)
{
  return parsepile_expect(parser, TOKEN_TYPE_LPAREN)
      && parsepile_expression(parser, compiler)
      && parsepile_expect(parser, TOKEN_TYPE_RPAREN);
}

bool parsepile_block_body(struct parser* parser, struct compiler* compiler) {
  while (!parser_check(parser, TOKEN_TYPE_RCURLY)) {
    if (!parsepile_instruction(parser, compiler))
      return false;
  }
  return true;
}

bool parsepile_block(struct parser* parser, struct compiler* compiler) {
  struct compiler  subcompiler;
  bool             result;

  compiler_create_sub(&subcompiler, compiler);
  result = parsepile_block_body(parser, &subcompiler);
  compiler_destroy(&subcompiler);

  return result;
}

bool parsepile_if(struct parser* parser, struct compiler* compiler) {
  t_compiler_label middle;
  t_compiler_label end;
  bool             result;

  if (!parsepile_parenthesized_expression(parser, compiler))
    return false;

  middle = compiler_open_label(compiler);
  end    = compiler_open_label(compiler);

  result = false;

  compiler_jump_if_not(compiler, middle);
  if (parsepile_instruction(parser, compiler)) {
    compiler_jump(compiler, end);
    compiler_place_label(compiler, middle);
    result = true;
    if (parser_check(parser, TOKEN_TYPE_KW_ELSE)) {
      if (!parsepile_instruction(parser, compiler)) {
        result = false;
      }
    }
    compiler_place_label(compiler, end);
  }

  compiler_close_label(compiler, middle);
  compiler_close_label(compiler, end);

  return result;
}

bool parsepile_while(struct parser* parser, struct compiler* compiler) {
  t_compiler_label head;
  t_compiler_label end;
  struct compiler  subcompiler;
  bool             result;

  compiler_create_sub(&subcompiler, compiler);

  head = compiler_open_continue_label(&subcompiler);
  end  = compiler_open_break_label(&subcompiler);

  result = false;

  compiler_place_label(&subcompiler, head);

  if (parsepile_parenthesized_expression(parser, &subcompiler)) {
    compiler_jump_if_not(&subcompiler, end);
    if (parsepile_instruction(parser, &subcompiler)) {
      compiler_jump(&subcompiler, head);
      compiler_place_label(&subcompiler, end);
      result = true;
    }
  }

  compiler_close_label(&subcompiler, head);
  compiler_close_label(&subcompiler, end);

  compiler_destroy(&subcompiler);

  return result;
}

bool parsepile_do_while(struct parser* parser, struct compiler* compiler) {
  t_compiler_label head;
  t_compiler_label end;
  struct compiler  subcompiler;
  bool             result;

  compiler_create_sub(&subcompiler, compiler);

  head = compiler_open_continue_label(&subcompiler);
  end  = compiler_open_break_label(&subcompiler);

  result = false;

  compiler_place_label(&subcompiler, head);

  if (parsepile_instruction(parser, &subcompiler)) {
    if (parsepile_expect(parser, TOKEN_TYPE_KW_WHILE)) {
      if (parsepile_parenthesized_expression(parser, &subcompiler)) {
        compiler_jump_if(&subcompiler, head);
        result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
      }
    }
  }

  compiler_place_label(&subcompiler, end);

  compiler_close_label(&subcompiler, head);
  compiler_close_label(&subcompiler, end);

  compiler_destroy(&subcompiler);

  return result;
}

bool parsepile_for_init(struct parser* parser, struct compiler* compiler) {
  struct symbol* symbol;
  struct type*   type;
  bool           result;

  result = false;
  if (parse_type_and_name(parser, &type, &symbol)) {
    compiler_add_var(compiler, type, symbol);
    if (parsepile_expect(parser, TOKEN_TYPE_ASSIGNMENT)) {
      if (parsepile_expression(parser, compiler)) {
        result = parsepile_store_var(parser, compiler, symbol);
      }
    }
  } else {
    result = parsepile_expression(parser, compiler);
  }

  return result;
}

bool parsepile_for(struct parser* parser, struct compiler* compiler) {
  struct symbol*   symbol;
  struct type*     type;
  struct symbol*   list_var;
  struct symbol*   index_var;
  t_compiler_label head;
  t_compiler_label cont;
  t_compiler_label middle;
  t_compiler_label end;
  struct compiler  subcompiler;
  bool             iresult;
  bool             result;

  compiler_create_sub(&subcompiler, compiler);

  head   = compiler_open_label(&subcompiler);
  cont   = compiler_open_continue_label(&subcompiler);
  end    = compiler_open_break_label(&subcompiler);
  middle = compiler_open_label(&subcompiler);

  result = false;

  if (parsepile_expect(parser, TOKEN_TYPE_LPAREN)) {
     //&& parsepile_for_init(parser, &subcompiler)

     iresult = false;
     if (parse_type_and_name(parser, &type, &symbol)) {
       compiler_add_var(compiler, type, symbol);
       if (parser_check(parser, TOKEN_TYPE_COLON)) {
         /*
          * We now parse a for-each statement
          */
         list_var  = raven_gensym(parser->raven);
         index_var = raven_gensym(parser->raven);
         compiler_add_var(&subcompiler, NULL, list_var);
         compiler_add_var(&subcompiler, NULL, index_var);

         if (parsepile_expression(parser, &subcompiler)
          && parsepile_expect(parser, TOKEN_TYPE_RPAREN)) {
            compiler_store_var(&subcompiler, list_var);
            compiler_load_constant(&subcompiler, any_from_int(0));
            compiler_store_var(&subcompiler, index_var);

            compiler_place_label(&subcompiler, cont);
            compiler_load_var(&subcompiler, index_var);
            compiler_push(&subcompiler);
            compiler_load_var(&subcompiler, list_var);
            compiler_op(&subcompiler, RAVEN_OP_SIZEOF);
            compiler_op(&subcompiler, RAVEN_OP_LESS);
            compiler_jump_if_not(&subcompiler, end);

            compiler_load_var(&subcompiler, list_var);
            compiler_push(&subcompiler);
            compiler_load_var(&subcompiler, index_var);
            compiler_push(&subcompiler);
            compiler_push(&subcompiler);
            compiler_load_constant(&subcompiler, any_from_int(1));
            compiler_op(&subcompiler, RAVEN_OP_ADD);
            compiler_store_var(&subcompiler, index_var);
            compiler_pop(&subcompiler);
            compiler_op(&subcompiler, RAVEN_OP_INDEX);
            compiler_store_var(&subcompiler, symbol);

            result = parsepile_instruction(parser, &subcompiler);

            compiler_jump(&subcompiler, cont);
            compiler_place_label(&subcompiler, end);
         }
         // TODO: Iresult is false, result may be true
       } else if (parsepile_expect(parser, TOKEN_TYPE_ASSIGNMENT)) {
         if (parsepile_expression(parser, compiler)) {
           iresult = parsepile_store_var(parser, compiler, symbol);
         }
       }
     } else {
       iresult = parsepile_expression(parser, compiler);
     }

     if (iresult && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON)) {
      compiler_place_label(&subcompiler, head);
      if (parsepile_expression(parser, &subcompiler)) {
        compiler_jump_if_not(&subcompiler, end);
        compiler_jump(&subcompiler, middle);
        if (parsepile_expect(parser, TOKEN_TYPE_SEMICOLON)) {
          compiler_place_label(&subcompiler, cont);
          if (parsepile_expression(parser, &subcompiler)) {
            compiler_jump(&subcompiler, head);
            if (parsepile_expect(parser, TOKEN_TYPE_RPAREN)) {
              compiler_place_label(&subcompiler, middle);
              if (parsepile_instruction(parser, &subcompiler)) {
                compiler_jump(&subcompiler, cont);
                compiler_place_label(&subcompiler, end);
                result = true;
              }
            }
          }
        }
      }
    }
  }

  compiler_close_label(&subcompiler, head);
  compiler_close_label(&subcompiler, cont);
  compiler_close_label(&subcompiler, middle);
  compiler_close_label(&subcompiler, end);

  compiler_destroy(&subcompiler);

  return result;
}

bool parsepile_return(struct parser* parser, struct compiler* compiler) {
  if (parsepile_expression(parser, compiler)) {
    return parsepile_return_with_typecheck(parser, compiler)
        && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  }
  return false;
}


bool parsepile_instruction(struct parser* parser, struct compiler* compiler) {
  struct type*    type;
  struct symbol*  name;
  bool            result;

  result = false;
  parser_reset_exprtype(parser);
  if (parse_type_and_name(parser, &type, &name)) {
    compiler_add_var(compiler, type, name);
    if (parser_check(parser, TOKEN_TYPE_ASSIGNMENT)) {
      if (parsepile_expression(parser, compiler)) {
        result = parsepile_store_var(parser, compiler, name);
      }
    } else {
      result = true;
    }
    result = result && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  } else if (parser_check(parser, TOKEN_TYPE_LCURLY)) {
    result = parsepile_block(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_KW_IF)) {
    result = parsepile_if(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_KW_WHILE)) {
    result = parsepile_while(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_KW_DO)) {
    result = parsepile_do_while(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_KW_FOR)) {
    result = parsepile_for(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_KW_BREAK)) {
    compiler_break(compiler);
    result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  } else if (parser_check(parser, TOKEN_TYPE_KW_CONTINUE)) {
    compiler_continue(compiler);
    result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  } else if (parser_check(parser, TOKEN_TYPE_KW_RETURN)) {
    result = parsepile_return(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_SEMICOLON)) {
    result = true;
  } else {
    result = parsepile_expression(parser, compiler)
          && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  }

  return result;
}


bool parsepile_arglist(struct parser* parser, struct compiler* compiler) {
  struct type*    type;
  struct symbol*  name;

  if (parser_check(parser, TOKEN_TYPE_RPAREN))
    return true;
  else {
    while (true) {
      if (parser_check(parser, TOKEN_TYPE_ELLIPSIS)) {
        compiler_enable_varargs(compiler);
        return parsepile_expect(parser, TOKEN_TYPE_RPAREN);
      }
      if (!parse_type_and_name(parser, &type, &name))
        return false;
      compiler_add_arg(compiler, type, name);
      compiler_load_var(compiler, name);
      compiler_typecheck(compiler, type);
      if (parser_check(parser, TOKEN_TYPE_RPAREN))
        return true;
      if (!parsepile_expect(parser, TOKEN_TYPE_COMMA))
        return false;
    }
  }
}


bool parsepile_file_statement(struct parser* parser, struct blueprint* into) {
  struct codewriter  codewriter;
  struct compiler    compiler;
  struct type*       type;
  struct symbol*     name;
  struct function*   function;
  bool               result;

  result = false;

  if (parse_type_and_name(parser, &type, &name)) {
    if (parser_check(parser, TOKEN_TYPE_LPAREN)) {
      /*
       * We're parsing a function
       */
      parser_set_returntype(parser, type);

      codewriter_create(&codewriter, parser->raven);  /* This is very hacky! */
      compiler_create(&compiler, &codewriter, into);

      if (parsepile_arglist(parser, &compiler)) {
        if (parsepile_expect(parser, TOKEN_TYPE_LCURLY)) {
          if (parsepile_block_body(parser, &compiler)) {
            function = compiler_finish(&compiler);
            if (function != NULL) {
              blueprint_add_func(into, name, function);
              result = true;
            }
          }
        }
      }

      compiler_destroy(&compiler);
      codewriter_destroy(&codewriter);
    } else {
      /*
       * We're parsing a variable
       */
      if (parsepile_expect(parser, TOKEN_TYPE_SEMICOLON)) {
        blueprint_add_var(into, type, name);
        result = true;
      }
    }
  }

  return result;
}

bool parsepile_inheritance(struct parser* parser, struct blueprint* into) {
  struct blueprint*  bp;
  bool               result;

  result = false;

  if (parser_check(parser, TOKEN_TYPE_KW_INHERIT)) {
    /* 'inherit;' inherits none */
    if (parser_check(parser, TOKEN_TYPE_SEMICOLON))
      result = true;
    else if (parsepile_expect_noadvance(parser, TOKEN_TYPE_STRING)) {
      bp = parser_as_relative_blueprint(parser, into);
      if (bp != NULL) {
        if (blueprint_inherit(into, bp)) {
          parser_advance(parser);
          result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
        }
      }
    }
  } else {
    bp = raven_get_blueprint(parser->raven, "/std/base.c");
    if (bp != NULL && blueprint_inherit(into, bp)) {
      result = true;
    }
  }

  return result;
}

bool parsepile_file(struct parser* parser, struct blueprint* into) {
  bool  result;

  if (!parsepile_inheritance(parser, into))
    return false;
  else {
    result = true;

    while (!parser_is(parser, TOKEN_TYPE_EOF)) {
      if (!parsepile_file_statement(parser, into)) {
        result = false;
        break;
      }
    }

    return result;
  }
}
