/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

/*
 * This file is probably one of the longest in this project, as it
 * implements the entire parser and compiler of the LPC language used
 * for this MUD.
 *
 * Contrary to other projects like e.g. LDMUD I decided against
 * using bison or yacc, for various reasons:
 *
 *  - Writing a parser and compiler for LPC is relatively easy.
 *  - Adding a build dependency binds my project to the availability
 *    of another program that is not entirely under my control. It
 *    also forces everyone who's trying to compile Raven to install
 *    more external software. We can do better than that.
 *  - Yacc and bison are more complex than Raven itself, therefore
 *    increasing the "invisible lines of code" behind Raven drastically.
 *  - Pure C is easier to read and takes less time to adjust to.
 *
 * However, I must say that the design of the parsepiler is not as
 * elegant as I want it to be. A lot of functionality was crammed
 * into the `parser` structure, which acts as the invisible statekeeper
 * of the parsepilation process. At this point it shouldn't be called
 * a `parser` anymore, but I thought that the name `parsepiler` blew up
 * a lot of lines to beyond the 80-column limit, while neither improving
 * readability nor providing any increased sense of simplicity.
 *
 * This is the way the parsepiler works:
 * Most functions in this file have two arguments, a `struct parser`,
 * and a `struct compiler`. The `parser` is responsible for breaking
 * the input down into tokens, and giving us an interface to examine
 * and convert the current token into any type we need, while the
 * `compiler` keeps track of the current scope we're in (i.e. what
 * variables are available at the moment) and receives the bytecodes
 * that we want to generate - which then get forwarded to an object
 * that records them for later conversion into a function instance.
 *
 * The parsepiler starts out with a `parser` that is holding the data
 * of the first token in the LPC file:
 *
 *     inherit "/std/object";
 *     ^
 *     |
 *     +- The parser points to this token!
 *
 *
 * We can then "check" on the token type of the currently visible
 * token:
 *
 *     if (parser_check(parser, TOKEN_TYPE_KW_INHERIT)) {
 *       // The first token is the keyword "inherit"
 *     } else if (parser_check(parser, TOKEN_TYPE_IDENT)) {
 *       // The first token is an identifier (like `x`)
 *     } else {
 *       // Something different...
 *     }
 *
 * If the token is of the specified type, the `parser_check(...)`
 * function returns `true` and causes the parser to advance to the
 * next token (although there are ways to just get the type without
 * advancing to the next token - this is especially useful if the
 * token has some sort of attached data, like the value of a number
 * token that needs to be fetched before advancing).
 *
 * These statements can then be grouped into more complex "check"
 * functions that instead of checking for individual keywords check
 * for entire language constructs.
 *
 * In LPC, most of these language constructs have a direct equivalent
 * in bytecode. Therefore, while checking for certain keywords and
 * details of such a construct (e.g. whether an `if` statement also
 * contains an `else` branch) we can also emit the related bytecode
 * in the meantime. Therefore, a "check" is often followed by a
 * command to the `compiler` structure, telling it which instruction
 * to append next.
 *
 * The functions responsible for all of this usually return a `bool`,
 * describing whether their execution was successful:
 *
 *     {
 *       bool  result;
 *
 *       result = false
 *       if (parsepile_block(parser, compiler)) {
 *         // etc. ...
 *         result = true;
 *       }
 *       return result;
 *     }
 *
 * The reason for that is that C doesn't have a builtin mechanism for
 * throwing an exception when a syntax error comes up. Therefore,
 * every function is required to communicate success or failure of
 * its execution through its return value.
 * I am of course aware that `setjmp(...)` and `longjmp(...)` exist.
 * However, I consider them to be an ugly solution since they allow
 * the CPU to jump directly up in the callstack and therefore skip
 * some (or all) destructor functions that are still pending.
 * Other languages like C++ take care of that in their exception
 * system, but since C has no concept of destructors this is a bug
 * waiting to happen.
 *
 * Also, a note about the type system:
 * The parsepiler supports typechecking some expressions. Since every
 * variable also has a pointer to its type the parsepiler can compare
 * the expected type to the type of the expression that was just
 * parsed and compiled. It uses a special pointer inside of the
 * `parser` structure for this. When an expression gets parsed, its
 * (sometimes inferred) return type is stored in the parser. When
 * the compiled value then gets assigned to a variable, its type
 * is compared to the variable's type, and if there's a clear mismatch
 * a warning will be sent to the compilation log (which is also stored
 * in the `parser`).
 */

#include "../defs.h"

#include "../core/objects/function.h"
#include "../fs/file.h"
#include "../util/stringbuilder.h"

#include "bytecodes.h"
#include "modifiers.h"
#include "codewriter.h"
#include "compiler.h"

#include "parsepiler.h"


bool parsepile_file_impl(struct parser*    parser,
                         struct blueprint* into,
                         bool              inheritance,
                         enum   token_type stop);


void parser_error(struct parser* parser, const char* format, ...) {
  va_list args;

  va_start(args, format);
  log_vprintf_error(parser_log(parser),
                    "LPC Source Code (file unknown)",
                    parser_src(parser),
                    parser_line(parser),
                    parser_caret(parser),
                    format,
                    args);
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

bool parse_modifier(struct parser* parser, enum raven_modifier* loc) {
  if (parser_check(parser, TOKEN_TYPE_KW_PRIVATE)) {
    *loc = RAVEN_MODIFIER_PRIVATE;
  } else if (parser_check(parser, TOKEN_TYPE_KW_PROTECTED)) {
    *loc = RAVEN_MODIFIER_PROTECTED;
  } else {
    return false;
  }
  return true;
}

bool parse_assignment_op(struct parser* parser, enum raven_op* op) {
  if (parser_check(parser, TOKEN_TYPE_PLUS_ASSIGNMENT)) {
    if (op != NULL) *op = RAVEN_OP_ADD;
  } else if (parser_check(parser, TOKEN_TYPE_MINUS_ASSIGNMENT)) {
    if (op != NULL) *op = RAVEN_OP_SUB;
  } else if (parser_check(parser, TOKEN_TYPE_STAR_ASSIGNMENT)) {
    if (op != NULL) *op = RAVEN_OP_MUL;
  } else if (parser_check(parser, TOKEN_TYPE_SLASH_ASSIGNMENT)) {
    if (op != NULL) *op = RAVEN_OP_DIV;
  } else if (parser_check(parser, TOKEN_TYPE_PERCENT_ASSIGNMENT)) {
    if (op != NULL) *op = RAVEN_OP_MOD;
  } else {
    return false;
  }
  return true;
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

bool expect_symbol(struct parser* parser, struct symbol** loc) {
  if (!parse_symbol(parser, loc)) {
    parser_error(parser, "Syntax error, expected an identifier!\n");
    return false;
  }
  return true;
}

bool parse_type(struct parser* parser, struct type** loc) {
  *loc = NULL;
  if (parser_check(parser, TOKEN_TYPE_KW_VOID))
    *loc = typeset_type_void(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_ANY))
    *loc = typeset_type_any(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_MIXED))
    *loc = typeset_type_any(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_BOOL))
    *loc = typeset_type_bool(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_INT))
    *loc = typeset_type_int(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_CHAR))
    *loc = typeset_type_char(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_STRING))
    *loc = typeset_type_string(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_SYMBOL))
    *loc = typeset_type_symbol(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_OBJECT))
    *loc = typeset_type_object(raven_types(parser_raven(parser)));
  else if (parser_check(parser, TOKEN_TYPE_KW_MAPPING))
    *loc = typeset_type_mapping(raven_types(parser_raven(parser)));
  else
    return false;

  while (true) {
    if (parser_check(parser, TOKEN_TYPE_STAR)) {
      /*
       * For now, we don't have array types yet, so we default
       * to `any`.
       */
      *loc = typeset_type_any(raven_types(parser_raven(parser)));
    } else if (parser_check(parser, TOKEN_TYPE_LPAREN)) {
      parser_check(parser, TOKEN_TYPE_ELLIPSIS);
      if (!parser_check(parser, TOKEN_TYPE_RPAREN))
        return false;
      *loc = typeset_type_funcref(raven_types(parser_raven(parser)));
    } else {
      break;
    }
  }

  return true;
}

bool parse_colon_type(struct parser* parser, struct type** loc) {
  if (parser_check(parser, TOKEN_TYPE_COLON))
    return parse_type(parser, loc);
  else {
    *loc = typeset_type_any(raven_types(parser_raven(parser)));
    return true;
  }
}

bool parse_type_and_name(struct parser*  parser,
                         struct type**   type,
                         struct symbol** name) {
  return parse_type(parser, type) && parse_symbol(parser, name);
}

bool parse_fancy_vardecl(struct parser*  parser,
                         struct type**   type,
                         struct symbol** name) {
  if (parse_type(parser, type))
    return parse_symbol(parser, name);
  else
    return parser_check(parser, TOKEN_TYPE_KW_LET)
        && parse_symbol(parser, name)
        && parse_colon_type(parser, type);
}

bool parsepile_load_var(struct parser*   parser,
                        struct compiler* compiler,
                        struct symbol*   name) {
  struct type*  type;
  bool          result;

  result = compiler_load_var_with_type(compiler, name, &type);
  if (result) parser_set_exprtype(parser, type);
  else parser_error(parser,
                    "Invalid variable name: %s!",
                    symbol_name(name));
  return result;
}

bool parsepile_store_var(struct parser*   parser,
                         struct compiler* compiler,
                         struct symbol*   name) {
  struct type*  type;
  bool          result;

  result = compiler_store_var_with_type(compiler, name, &type);
  if (!result) {
    parser_error(parser,
                 "Invalid variable name: %s!",
                 symbol_name(name));
  } else if (!type_match(type, parser_get_exprtype(parser))) {
    parser_error(parser, "Warning: possible type mismatch!\n");
  }
  return result;
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
  enum   raven_op op;
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
    } else if (parse_assignment_op(parser, &op)) {
      if (parsepile_load_var(parser, compiler, symbol)) {
        compiler_push(compiler);
        if (parsepile_expr(parser, compiler, pr)) {
          compiler_op(compiler, op);
          parser_set_exprtype_to_any(parser);
          result = parsepile_store_var(parser, compiler, symbol);
        }
      }
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
      if (parsepile_expression(parser, compiler)) {
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
  } else if (parser_is(parser, TOKEN_TYPE_SYMBOL)) {
    compiler_load_constant(compiler, any_from_ptr(parser_as_symbol(parser)));
    parser_advance(parser);
    result = true;
    parser_set_exprtype_to_symbol(parser);
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
    /*
     * This is the address operator, used in expressions
     * such as:
     *
     *     function f = &foo;
     *
     * which will return a function pointer to the function `foo`.
     */
    if (!parse_symbol(parser, &symbol))
      return false;
    compiler_load_funcref(compiler, symbol);
    parser_set_exprtype_to_any(parser); /* TODO: Function type */
  } else if (parser_check(parser, TOKEN_TYPE_STAR)) {
    /*
     * This is the dereference operator, used in expressions
     * such as:
     *
     *     *"/secure/master"
     *
     * which will reference the object `/secure/master`.
     *
     * In practice, this will just be a hidden call to the
     * method `the(...)`.
     */
     compiler_push_self(compiler); /* TODO: Maybe push a special object? */
     if (!parsepile_expr(parser, compiler, 1))
       return false;
     compiler_push(compiler);
     compiler_send(compiler,
                   raven_find_symbol(parser->raven, "the"),
                   1);
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
  if (parse_fancy_vardecl(parser, &type, &symbol)) {
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
     iresult = false;
     if (parse_fancy_vardecl(parser, &type, &symbol)) {
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

bool parsepile_switch(struct parser* parser, struct compiler* compiler) {
  struct compiler   subcompiler;
  t_compiler_label  continuation;
  t_compiler_label  skip;
  t_compiler_label  end;
  bool              result;
  bool              has_default;

  result      = false;
  has_default = false;

  compiler_create_sub(&subcompiler, compiler);

  continuation = compiler_open_label(&subcompiler);
  end          = compiler_open_break_label(&subcompiler);

  if (parsepile_parenthesized_expression(parser, &subcompiler)) {
    if (parsepile_expect(parser, TOKEN_TYPE_LCURLY)) {
      result = true;
      compiler_push(&subcompiler);
      compiler_jump(&subcompiler, continuation);
      while (result && !parser_check(parser, TOKEN_TYPE_RCURLY)) {
        if (parser_check(parser, TOKEN_TYPE_KW_CASE)) {
          /*
           * We are currently in a normal stream of instructions.
           * This stream will be broken here to inject another
           * `case` statement.
           */
          skip = compiler_open_label(&subcompiler);
          compiler_jump(&subcompiler, skip);
          /*
           * Our decision path continues here. We place the label,
           * destroy it, and fill `continuation` immediately
           * with a new label (to which we jump if the comparison
           * fails).
           */
          compiler_place_label(&subcompiler, continuation);
          compiler_close_label(&subcompiler, continuation);
          continuation = compiler_open_label(&subcompiler);
          /*
           * Do a POP, followed by a PUSH. This will ensure that our
           * originally pushed value stays on the stack.
           */
          compiler_pop(&subcompiler);
          compiler_push(&subcompiler);
          /*
           * Push the value once more, this time as the first operand
           * to the comparison operation.
           */
          compiler_push(&subcompiler);
          /*
           * Load the expression that we want to compare our value
           * against.
           */
          result = parsepile_expression(parser, &subcompiler)
                && parsepile_expect(parser, TOKEN_TYPE_COLON);
          /*
           * Perform the actual comparison.
           */
          compiler_op(&subcompiler, RAVEN_OP_EQ);
          /*
           * We assume that the comparison succeeded. If it didn't,
           * we follow our comparison path to the next statement.
           */
          compiler_jump_if_not(&subcompiler, continuation);
          /*
           * Since the comparison succeeded, there's no need to keep
           * this value on the stack. Drop it!
           */
          compiler_pop(&subcompiler);
          /*
           * We can now place and close our label to resume execution.
           */
          compiler_place_label(&subcompiler, skip);
          compiler_close_label(&subcompiler, skip);
        } else if (parser_check(parser, TOKEN_TYPE_KW_DEFAULT)) {
          /*
           * This is the `default` statement.
           */
          has_default = true;
          /*
           * We are currently in a normal stream of instructions.
           * This stream will be broken here to inject the `default`
           * statement.
           */
          skip = compiler_open_label(&subcompiler);
          compiler_jump(&subcompiler, skip);
          /*
           * Our decision path continues here. We place the label,
           * destroy it, and fill `continuation` immediately
           * with a new label (to which we jump if the comparison
           * fails).
           */
          compiler_place_label(&subcompiler, continuation);
          compiler_close_label(&subcompiler, continuation);
          continuation = compiler_open_label(&subcompiler);
          /*
           * We expect the colon, of course.
           */
          result = parsepile_expect(parser, TOKEN_TYPE_COLON);
          /*
           * Pop the calculated value from the first statement.
           */
          compiler_pop(&subcompiler);
          /*
           * We can now place and close our label to resume execution.
           */
          compiler_place_label(&subcompiler, skip);
          compiler_close_label(&subcompiler, skip);
        } else {
          result = parsepile_instruction(parser, &subcompiler);
        }
      }

      /*
       * We are in the normal path, jump to the end of the statement.
       */
      compiler_jump(&subcompiler, end);

      /*
       * This is the end of the normal decision path.
       */
      compiler_place_label(&subcompiler, continuation);
      if (!has_default) {
        /*
         * Pop the calculated value from the first statement.
         */
        compiler_pop(&subcompiler);
      }

      /*
       * This is the end of our switch statement.
       */
      compiler_place_label(&subcompiler, continuation);
      compiler_place_label(&subcompiler, end);
    }
  }

  compiler_close_label(&subcompiler, end);
  compiler_close_label(&subcompiler, continuation);

  compiler_destroy(&subcompiler);

  return result;
}

bool parsepile_return(struct parser* parser, struct compiler* compiler) {
  if (parser_check(parser, TOKEN_TYPE_SEMICOLON)) {
    compiler_load_constant(compiler, any_nil());
    parser_set_exprtype_to_void(parser);
    return parsepile_return_with_typecheck(parser, compiler);
  } else if (parsepile_expression(parser, compiler)) {
    return parsepile_return_with_typecheck(parser, compiler)
        && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  }
  return false;
}

bool parsepile_trycatch(struct parser* parser, struct compiler* compiler) {
  struct type*      type;
  struct symbol*    name;
  struct compiler   subcompiler;
  t_compiler_label  label;
  bool              result;
  bool              result2;

  compiler_create_sub(&subcompiler, compiler);

  if (!compiler_open_catch(&subcompiler)) {
    /* TODO: Error */
    return false;
  }

  result  = false;
  result2 = true;

  if (parsepile_instruction(parser, &subcompiler)) {
    if (parsepile_expect(parser, TOKEN_TYPE_KW_CATCH)) {
       label = compiler_open_label(&subcompiler);
       compiler_jump(&subcompiler, label);
       compiler_place_catch(&subcompiler);
       if (parser_check(parser, TOKEN_TYPE_LPAREN)) {
        result2 = false;
        if (parse_fancy_vardecl(parser, &type, &name)) {
          compiler_add_var(&subcompiler, type, name);
          compiler_typecheck(&subcompiler, type);
          compiler_store_var(&subcompiler, name);
          result2 = parsepile_expect(parser, TOKEN_TYPE_RPAREN);
        }
      }
      if (parsepile_instruction(parser, &subcompiler)) {
        result = result2;
      }
      compiler_place_label(&subcompiler, label);
      compiler_close_label(&subcompiler, label);
    }
  }

  compiler_destroy(&subcompiler);

  return result;
}

/*
 * Parse an instruction.
 */
bool parsepile_instruction(struct parser* parser, struct compiler* compiler) {
  struct type*    type;
  struct symbol*  name;
  bool            result;

  result = false;
  parser_reset_exprtype(parser);
  if (parse_fancy_vardecl(parser, &type, &name)) {
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
  } else if (parser_check(parser, TOKEN_TYPE_KW_SWITCH)) {
    result = parsepile_switch(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_KW_BREAK)) {
    compiler_break(compiler);
    result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  } else if (parser_check(parser, TOKEN_TYPE_KW_CONTINUE)) {
    compiler_continue(compiler);
    result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
  } else if (parser_check(parser, TOKEN_TYPE_KW_RETURN)) {
    result = parsepile_return(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_KW_TRY)) {
    result = parsepile_trycatch(parser, compiler);
  } else if (parser_check(parser, TOKEN_TYPE_SEMICOLON)) {
    result = true;
  } else {
    if (parsepile_expression(parser, compiler)) {
      result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
    } else {
      parser_error(parser, "Expected an instruction!");
    }
  }

  return result;
}

/*
 * Parse a script, which is a list of instructions.
 */
bool parsepile_script(struct parser* parser, struct compiler* compiler) {
  while (!parser_check(parser, TOKEN_TYPE_EOF)) {
    if (!parsepile_instruction(parser, compiler))
      return false;
  }
  return true;
}

/*
 * Parse an argument list of the form (type1 name1, type2 name2, ...).
 *
 * We also handle the ellipsis (`...`) for optional arguments. The
 * arguments will be forwarded to the compiler, which will then
 * store them in the generated function object.
 */
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
      if (!parse_type_and_name(parser, &type, &name)) {
        parser_error(parser, "Expected a type and name!");
        return false;
      }
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


/*
 * Compile file statements, like variables or functions.
 *
 * Usually, a file statement begins with a type designator (like `int`),
 * followed by a symbol (the name). If an open parenthesis follows,
 * we know that we're processing a function. If it doesn't, we
 * assume that we're loading a member variable.
 */
bool parsepile_file_statement(struct parser*    parser,
                              struct blueprint* into,
                              struct compiler*  init_compiler) {
  struct codewriter      codewriter;
  struct compiler        compiler;
  struct type*           type;
  struct symbol*         name;
  struct function*       function;
  enum   raven_modifier  modifier;
  bool                   result;

  result = false;

  if (!parse_modifier(parser, &modifier)) {
    modifier = RAVEN_MODIFIER_NONE;
  }

  if (parse_type_and_name(parser, &type, &name)) {
    if (parser_check(parser, TOKEN_TYPE_LPAREN)) {
      /*
       * We're parsing a function
       */
      parser_set_returntype(parser, type);

      /* Extracting `raven` this way is very hacky! */
      codewriter_create(&codewriter, parser->raven);
      compiler_create(&compiler, parser->raven, &codewriter, into);

      if (parsepile_arglist(parser, &compiler)) {
        if (parsepile_expect(parser, TOKEN_TYPE_LCURLY)) {
          if (parsepile_block_body(parser, &compiler)) {
            function = compiler_finish(&compiler);
            if (function != NULL) {
              function_set_modifier(function, modifier);
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
      blueprint_add_var(into, type, name);
      result = true;
      if (parser_check(parser, TOKEN_TYPE_ASSIGNMENT)) {
        result = parsepile_expression(parser, init_compiler)
              && parsepile_store_var(parser, init_compiler, name);
      }
      result = result && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
    }
  } else {
    parser_error(parser, "Invalid toplevel expression");
  }

  return result;
}

/*
 * Process an `inherit` statement.
 *
 * This function will recursively load and include all referenced
 * blueprints.
 *
 * If no `inherit` statement was given, we inherit from "/secure/base".
 */
bool parsepile_inheritance(struct parser*    parser,
                           struct blueprint* into,
                           struct compiler*  init_compiler) {
  struct blueprint*  bp;
  bool               has_inheritance;
  bool               result;

  has_inheritance = true;
  result          = false;

  if (parser_check(parser, TOKEN_TYPE_KW_INHERIT)) {
    /* 'inherit;' inherits from nothing - needed for "/secure/base" itself */
    if (parser_check(parser, TOKEN_TYPE_SEMICOLON)) {
      has_inheritance = false;
      result          = true;
    } else if (parsepile_expect_noadvance(parser, TOKEN_TYPE_STRING)) {
      bp = parser_as_relative_blueprint(parser, into);
      if (bp != NULL) {
        if (blueprint_inherit(into, bp)) {
          parser_advance(parser);
          result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
        } else {
          parser_error(parser, "Inheritance failed!");
        }
      } else {
        parser_error(parser, "File not found!");
      }
    }
  } else {
    bp = raven_get_blueprint(parser->raven, "/secure/base");
    if (bp != NULL && blueprint_inherit(into, bp)) {
      result = true;
    }
  }

  if (has_inheritance) {
    /*
     * We do inherit from a different blueprint, so we instruct the
     * `_init` function to call up `::_init()`.
     */
    compiler_push_self(init_compiler);
    compiler_super_send(init_compiler,
                        raven_find_symbol(parser_raven(parser), "_init"),
                        0);
  }

  return result;
}


bool parsepile_include_statement(struct parser*    parser,
                                 struct blueprint* into) {
  struct file*          file;
  struct stringbuilder  sb;
  struct reader         reader;
  struct parser         new_parser;
  bool                  result;

  result = false;
  if (parsepile_expect_noadvance(parser, TOKEN_TYPE_STRING)) {
    file = file_resolve_flex(file_parent(blueprint_file(into)),
                             parser_as_cstr(parser));
    if (file != NULL) {
      parser_advance(parser);
      stringbuilder_create(&sb);
      file_cat(file, &sb);
      {
        reader_create(&reader, stringbuilder_get_const(&sb));
        parser_create(&new_parser, parser_raven(parser), &reader, parser_log(parser));
        result = parsepile_file_impl(&new_parser, into, false, TOKEN_TYPE_EOF);
        parser_destroy(&new_parser);
        reader_destroy(&reader);
      }
      stringbuilder_destroy(&sb);
    } else {
      parser_error(parser, "File not found!");
    }
  }

  return result;
}


/*
 * Parse a 'class' statement.
 *
 * Class statements allow to implement blueprints inside
 * of other blueprints. This is very useful for inlined
 * objects.
 *
 * A class statement looks like this:
 *
 *     class foo {
 *       inherit "/std/thing";
 *
 *       int bar = 42;
 *
 *       void baz() {
 *         // ...
 *       }
 *     };
 */
bool parsepile_class_statement(struct parser*    parser,
                               struct blueprint* into,
                               struct compiler*  compiler) {
  struct typeset*    types;
  struct symbol*     name;
  struct blueprint*  blue;
  struct object*     object;
  bool               result;

  result = false;

  if (expect_symbol(parser, &name)
   && parsepile_expect(parser, TOKEN_TYPE_LCURLY)) {
    blue = blueprint_new(parser_raven(parser), NULL);
    if (blue != NULL) {
      if (parsepile_file_impl(parser, blue, NULL, TOKEN_TYPE_RCURLY)) {
        /*
         * TODO, FIXME, XXX: Check for NULL return value!
         */
        object = blueprint_instantiate(blue, parser_raven(parser));
        types  = raven_types(parser_raven(parser));
        blueprint_add_var(into, typeset_type_object(types), name);
        compiler_load_constant(compiler, any_from_ptr(object));
        compiler_store_var(compiler, name);
        result = parsepile_expect(parser, TOKEN_TYPE_RCURLY)
              && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
      }
    }
  }
  return result;
}


/*
 * Parse an entire file.
 *
 * A file consists of a statement of the form:
 *
 *     inherit "...";
 *
 * followed by one or more variable declarations:
 *
 *     string name;
 *
 * and function definitions:
 *
 *     string query_name() {
 *       return name;
 *     }
 *
 * All of these statements will be compiled into the blueprint `into`,
 * which we assume to be freshly created.
 */
bool parsepile_file_impl(struct parser*    parser,
                         struct blueprint* into,
                         bool              inheritance,
                         enum token_type   stop) {
  struct codewriter  codewriter;
  struct compiler    compiler;
  struct function*   init_function;
  bool               result;

  result = false;

  codewriter_create(&codewriter, parser_raven(parser));
  compiler_create(&compiler, parser_raven(parser), &codewriter, into);

  if (!inheritance || parsepile_inheritance(parser, into, &compiler)) {
    result = true;

    while (!parser_is(parser, stop)
        && !parser_is(parser, TOKEN_TYPE_EOF)) {
      if (parser_check(parser, TOKEN_TYPE_KW_INCLUDE)) {
        if (!parsepile_include_statement(parser, into)) {
          result = false;
          break;
        }
      } else if (parser_check(parser, TOKEN_TYPE_KW_CLASS)) {
        if (!parsepile_class_statement(parser, into, &compiler)) {
          result = false;
          break;
        }
      } else if (!parsepile_file_statement(parser, into, &compiler)) {
        result = false;
        break;
      }
    }
  }

  init_function = compiler_finish(&compiler);

  if (init_function != NULL) {
    blueprint_add_func(into,
                       raven_find_symbol(parser_raven(parser), "_init"),
                       init_function);
  }

  compiler_destroy(&compiler);
  codewriter_destroy(&codewriter);

  return result;
}

bool parsepile_file(struct parser*    parser,
                    struct blueprint* into) {
  return parsepile_file_impl(parser, into, true, TOKEN_TYPE_EOF);
}

