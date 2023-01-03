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

#include "../../defs.h"
#include "../../platform/fs/fs_pather.h"
#include "../../platform/fs/fs.h"
#include "../../util/stringbuilder.h"

#include "../core/objects/function.h"

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
                      (parser_file_name(parser) == NULL) ? "LPC Source Code (file unknown)" : parser_file_name(parser),
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
    } else if (parser_check(parser, TOKEN_TYPE_KW_PUBLIC)) {
        *loc = RAVEN_MODIFIER_PUBLIC;
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

bool parse_operator_symbol(struct parser* parser, struct symbol** loc) {
    struct stringbuilder  sb;
    bool                  result;

    result = true;

    stringbuilder_create(&sb);
    {
        stringbuilder_append_str(&sb, "operator");

        if (parser_check_cstr(parser, "<<")) {
            stringbuilder_append_str(&sb, "<<");
        } else if (parser_check_cstr(parser, ">>")) {
            stringbuilder_append_str(&sb, ">>");
        } else {
            result = false;
        }

        if (result) {
            *loc = raven_find_symbol(parser_raven(parser), stringbuilder_get_const(&sb));
            parser_advance(parser);
        }
    }
    stringbuilder_destroy(&sb);

    return result;
}

bool parse_symbol(struct parser* parser, struct symbol** loc) {
    if (parser_is(parser, TOKEN_TYPE_KW_OPERATOR))
        return parse_operator_symbol(parser, loc);
    else if (!parser_is(parser, TOKEN_TYPE_IDENT))
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
        } else if (parser_check(parser, TOKEN_TYPE_LBRACK)) {
            if (!parsepile_expect(parser, TOKEN_TYPE_RBRACK))
                return false;

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
            if (symbol_is_builtin(symbol)) {
                compiler_call_builtin(compiler, symbol, argcount);
            } else {
                compiler_send(compiler, symbol, argcount);
            }
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
        /*
         * TODO, FIXME: This is not executed!
         */
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
    } else if (parser_check(parser, TOKEN_TYPE_ELLIPSIS)) {
        result = true;
        compiler_op(compiler, RAVEN_OP_ARGS);
        parser_set_exprtype_to_array(parser);
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


/*
 * Parse a "null aware" operator, which is an operator inspired by dart.
 *
 * The nullaware operator is written like this:
 * 
 *     expr1 ?? expr2
 * 
 * and is more-or-less equivalent to:
 * 
 *     expr1 ? expr1 : expr2
 */
bool parsepile_nullaware(struct parser* parser, struct compiler* compiler) {
    t_compiler_label  end;
    bool              result;

    end    = compiler_open_label(compiler);
    result = false;

    compiler_jump_if(compiler, end);

    result = parsepile_expr(parser, compiler, 12);

    compiler_place_label(compiler, end);

    compiler_close_label(compiler, end);

    parser_set_exprtype_to_any(parser); /* TODO: Infer */

    return result;
}

bool parsepile_op(struct parser*   parser,
                  struct compiler* compiler,
                  int              pr,
                  bool*            should_continue) {
    struct symbol*  symbol;
    struct type*    type;
    unsigned int    args;
    bool            result;

    *should_continue = true;
    result          = true;

    if ((pr >= 1 && parser_check(parser, TOKEN_TYPE_ARROW)) || (pr >= 15 && parser_check(parser, TOKEN_TYPE_PARROW))) {
        result = false;
        compiler_push(compiler);
        if (parse_symbol(parser, &symbol)
            && parsepile_expect(parser, TOKEN_TYPE_LPAREN)) {
            result = parsepile_args(parser, compiler, &args, TOKEN_TYPE_RPAREN);
            compiler_send(compiler, symbol, args);
        }
        parser_set_exprtype_to_any(parser); /* TODO: Infer */
    } else if (pr >= 1 && parser_check(parser, TOKEN_TYPE_DOT)) {
        result = false;
        compiler_op(compiler, RAVEN_OP_DEREF);
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
    } else if (pr >= 13 && parser_check(parser, TOKEN_TYPE_QUESTIONQUESTION)) {
        result = parsepile_nullaware(parser, compiler);
    } else if (pr >= 12 && parser_check(parser, TOKEN_TYPE_OR)) {
        result = parsepile_or(parser, compiler);
    } else if (pr >= 11 && parser_check(parser, TOKEN_TYPE_AND)) {
        result = parsepile_and(parser, compiler);
    } else if (pr >= 10 && parser_check(parser, TOKEN_TYPE_PIPE)) {
        compiler_push(compiler);
        result = parsepile_expr(parser, compiler, 9);
        compiler_op(compiler, RAVEN_OP_BITOR);
        parser_set_exprtype_to_any(parser); /* TODO: Infer */
    } else if (pr >= 8 && parser_check(parser, TOKEN_TYPE_AMPERSAND)) {
        compiler_push(compiler);
        result = parsepile_expr(parser, compiler, 7);
        compiler_op(compiler, RAVEN_OP_BITAND);
        parser_set_exprtype_to_any(parser); /* TODO: Infer */
    } else if (pr >= 5 && parser_check(parser, TOKEN_TYPE_LEFTSHIFT)) {
        compiler_push(compiler);
        result = parsepile_expr(parser, compiler, 4);
        compiler_op(compiler, RAVEN_OP_LEFTSHIFT);
        parser_set_exprtype_to_any(parser); /* TODO: Infer */
    } else if (pr >= 5 && parser_check(parser, TOKEN_TYPE_RIGHTSHIFT)) {
        compiler_push(compiler);
        result = parsepile_expr(parser, compiler, 4);
        compiler_op(compiler, RAVEN_OP_RIGHTSHIFT);
        parser_set_exprtype_to_any(parser); /* TODO: Infer */
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
    } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_KW_IS)) {
        compiler_push(compiler);
        result = parse_type(parser, &type);
        compiler_typeis(compiler, type);
        parser_set_exprtype_to_int(parser);
    } else {
        *should_continue = false;
    }

    return result;
}

/*
 * Parsepile an expression with a given precedence.
 *
 * Expressions are all of the following:
 *    - Function calls, e.g. `foo()`, `bar(1, 2, 3)`, `...->foo()`, `...->bar(1, 2, 3)`
 *    - Variable references, e.g. `x`, `index`, `foo`
 *    - Literals, e.g. `42`, `"Hello, world!"`, `true`
 *    - Unary and binary operators, e.g. `!x`, `x + y`, `x * y`, `x == y`, `x < y`, `x >> y`
 *    - Type checks, e.g. `x is int`, `x is string`
 *    - Function references, e.g. `&foo`, `&bar`
 *    - Dereferencing, e.g. `*"/secure/master"`
 *    - Array and map literals, e.g. `{1, 2, 3}`, `["foo": 1, "bar": 2]`
 *    - Array and map indexing, e.g. `x[0]`, `x["foo"]`
 * 
 * Since parsing these structures is too much for one function, we
 * only do a small fraction of the parsing here. Most of the interesting
 * stuff is done in the other `parsepile_*` functions.
 * 
 * NOTE: The precedence of each operator is almost the same as in C.
 * 
 */
bool parsepile_expr(struct parser* parser, struct compiler* compiler, int pr) {
    bool            should_continue;
    struct symbol*  symbol;

    /*
     * We are now in front of an expression, so we handle the
     * prefix operators first:
     */
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
        if (!parsepile_expr(parser, compiler, 1))
            return false;
        compiler_op(compiler, RAVEN_OP_DEREF);
        parser_set_exprtype_to_any(parser);
    } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_PLUS)) {
        /*
         * This is the unary plus operator, used in expressions
         * such as:
         * 
         *     +42
         *     +foo
         * 
         * which will just return the value of the expression.
         */
        return parsepile_expr(parser, compiler, 1);
    } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_MINUS)) {
        /*
         * This is the unary minus operator, used in expressions
         * such as:
         * 
         *     -42
         *     -foo
         * 
         * which will negate the value of the expression.
         */
        if (!parsepile_expr(parser, compiler, 1))
            return false;
        compiler_op(compiler, RAVEN_OP_NEGATE);
    } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_NOT)) {
        /*
         * This is the logical not operator, used in expressions
         * such as:
         * 
         *     !x
         *     !foo
         * 
         * which will negate the value of the expression.
         */
        if (!parsepile_expr(parser, compiler, 1))
            return false;
        compiler_op(compiler, RAVEN_OP_NOT);
    } else if (pr >= 2 && parser_check(parser, TOKEN_TYPE_KW_SIZEOF)) {
        /*
         * This is the sizeof operator, used in expressions
         * such as:
         * 
         *     sizeof(x)
         *     sizeof(foo)
         *     sizeof("Hello, world!")
         *     sizeof({1, 2, 3})
         * 
         * which will return the size of the computed value.
         */
        if (!parsepile_expr(parser, compiler, 1))
            return false;
        compiler_op(compiler, RAVEN_OP_SIZEOF);
    } else if (!parsepile_simple_expr(parser, compiler, pr)) {
        /*
         * Above this comment we parse the actual expression.
         *
         * Usually, an expression starts with a so-called "simple expression",
         * which is either a literal, a variable reference, or a function call.
         */
        return false;
    }

    /*
     * Here we handle the postfix operators.
     *
     * We look at the next token and decide if it's an operator. If it
     * is, we compile it and repeat until we reach the end of the expression
     * or an operator with a lower precedence (which will in turn be handled
     * by the parsepiler function that called us).
     */
    should_continue = true;
    while (should_continue) {
        if (!parsepile_op(parser, compiler, pr, &should_continue))
            return false;
    }

    /*
     * If we reach this point, we have successfully parsed the expression.
     *
     * Return true, and let the caller handle the rest. Cheerio.
     */
    return true;
}


/*
 * This function parses an expression.
 *
 * Essentially, this is just a wrapper around `parsepile_expr`, which
 * needs a magic precedence value to work. We chose 100 arbitrarily,
 * because no operator will have any precedence greater than that.
 * 
 * For more information, see the documentation for `parsepile_expr`.
 */
bool parsepile_expression(struct parser* parser, struct compiler* compiler) {
    return parsepile_expr(parser, compiler, 100);
}


/*
 * This function parses a parenthesized expression.
 *
 * A parenthesized expression is an expression surrounded by parentheses,
 * just like this:
 * 
 *     (x)
 *     (1 + 2)
 *
 * This is used in expressions such as:
 * 
 *    (1 + 2) * 3
 * 
 * but also in if statements, while loops, etc.:
 * 
 *    if (x == 42) {
 *       ...
 *    }
 */
bool parsepile_parenthesized_expression(struct parser*   parser,
                                        struct compiler* compiler)
{
    return parsepile_expect(parser, TOKEN_TYPE_LPAREN)
        && parsepile_expression(parser, compiler)
        && parsepile_expect(parser, TOKEN_TYPE_RPAREN);
}


/*
 * Parse the body of a block.
 *
 * A block is a sequence of instructions, surrounded by curly braces,
 * just like this:
 * 
 *     {
 *       ...
 *     }
 * 
 * This function parses the body of a block, which is everything between
 * the opening and closing curly braces.
 * 
 * The function returns true if the block was successfully parsed, and
 * false otherwise.
 * 
 * We parse the body of a block by repeatedly calling `parsepile_instruction`
 * until we reach the closing curly brace.
 * 
 * Note: We don't need to handle the opening curly brace here, because
 *       the caller will have already done that for us.
 */
bool parsepile_block_body(struct parser* parser, struct compiler* compiler) {
    while (!parser_check(parser, TOKEN_TYPE_RCURLY)) {
        if (!parsepile_instruction(parser, compiler))
            return false;
    }
    return true;
}


/*
 * Parse a block.
 *
 * A block is a sequence of instructions, surrounded by curly braces,
 * just like this:
 * 
 *     {
 *       ...
 *     }
 * 
 * This function establishes a new compiler context, and then calls
 * `parsepile_block_body` to parse the body of the block.
 * 
 * A new compiler context is established because with each block a new
 * scope is created, and we need to keep track of the variables declared
 * in that scope, and forget about them when the scope is destroyed
 * at the end of the block.
 * 
 * The function returns true if the block was successfully parsed, and
 * false otherwise.
 * 
 * Note: We don't need to handle the opening curly brace here, because
 *       the caller will have already done that for us.
 */
bool parsepile_block(struct parser* parser, struct compiler* compiler) {
    struct compiler  subcompiler;
    bool             result;

    compiler_create_sub(&subcompiler, compiler);
    result = parsepile_block_body(parser, &subcompiler);
    compiler_destroy(&subcompiler);

    return result;
}


/*
 * Parse an if statement.
 *
 * An if statement is a conditional statement, just like this:
 * 
 *     if (x == 42) {
 *       ...
 *     }
 * 
 * sometimes an optional else clause may follow:
 * 
 *     if (x == 42) {
 *       ...
 *     } else {
 *       ...
 *     }
 * 
 * Please note that the curly braces are not required, the actual
 * format of an if statement is:
 * 
 *    if (x == 42) ...
 * 
 * or:
 * 
 *   if (x == 42) ... else ...
 * 
 * where the "..." can be any instruction, including blocks
 * or other if statements.
 * 
 * If-else statements are just a special case of if statements, where
 * the else clause is just another if statement:
 * 
 *   if (x == 42) ...
 *   else <if (x == 43) ...>
 * 
 * This function parses an if statement, and returns true if the
 * statement was successfully parsed, and false otherwise.
 * 
 * The function works by first parsing the condition of the if statement,
 * which is the expression between the parentheses.
 * 
 * Since if statements are conditional statements, we need to make use
 * of the label facility of the compiler to implement the jumps.
 * 
 * An if statement translates to the following code:
 * 
 *     <condition>
 *     jump_if_not L1
 *     <then>
 *     jump L2
 * L1:
 *     <else>
 * L2:
 * 
 * where L1 and L2 are labels, and <condition>, <then> and <else> are
 * the instructions that make up the if statement.
 */
bool parsepile_if(struct parser* parser, struct compiler* compiler) {
    t_compiler_label middle;
    t_compiler_label end;
    bool             result;

    /*
     * Set up the return value.
     */
    result = false;

    /*
     * We start by parsing the condition of the if statement, which is
     * the expression between the parentheses.
     */
    if (!parsepile_parenthesized_expression(parser, compiler))
        return false;

    /*
     * We then open two labels, one for the middle of the if statement,
     * and one for the end of the if statement.
     */
    middle = compiler_open_label(compiler);
    end    = compiler_open_label(compiler);

    /*
     * We then jump to the middle of the if statement if the condition is
     * false.
     */
    compiler_jump_if_not(compiler, middle);

    /*
     * We then parsepile the then clause of the if statement.
     */
    if (parsepile_instruction(parser, compiler)) {
        /*
         * And jump to the end of the statement afterwards.
         */
        compiler_jump(compiler, end);

        /*
         * This is where the middle of the if statement will be placed.
         */
        compiler_place_label(compiler, middle);

        /*
         * We then check if there is an else clause.
         */
        result = true;
        if (parser_check(parser, TOKEN_TYPE_KW_ELSE)) {
            /*
             * If there is, we parsepile it.
             */
            if (!parsepile_instruction(parser, compiler)) {
                /*
                 * If the parsing failed, we set the return value back to false.
                 */
                result = false;
            }
        }

        /*
         * This is where the end of the if statement will be placed.
         */
        compiler_place_label(compiler, end);
    }

    /*
     * We then close the labels we opened.
     */
    compiler_close_label(compiler, middle);
    compiler_close_label(compiler, end);

    /*
     * And return the result.
     */
    return result;
}


/*
 * Parse a while statement.
 *
 * A while statement is a conditional statement, which allows us to
 * execute a sequence of instructions as long as a condition is true.
 * 
 * A while statement looks like this:
 * 
 *     while (x == 42) {
 *       ...
 *     }
 * 
 * This function parses a while statement, and returns true if the
 * statement was successfully parsed, and false otherwise.
 * 
 * The function works by first parsing the condition of the while statement,
 * which is the expression between the parentheses.
 * 
 * Since while statements are conditional statements, we need to make use
 * of the label facility of the compiler to implement the jumps.
 * 
 * A while statement translates to the following code:
 * 
 * L1:
 *     <condition>
 *     jump_if_not L2
 *     <body>
 *     jump L1
 * L2:
 * 
 * where L1 and L2 are labels, and <condition> and <body> are the
 * instructions that make up the while statement.
 * 
 * The while statement is implemented by jumping to the condition of the
 * statement, and then jumping to the end of the statement if the condition
 * is false.
 * 
 * The body of the while statement is then executed, and then we jump back
 * to the condition of the statement, and repeat the process.
 * 
 * The while statement is implemented using two labels, one for the
 * condition of the statement, and one for the end of the statement.
 * 
 * The body of the statement is placed between the two labels.
 * 
 * Since while statements also allow us to break out of the loop, we need
 * to open a break label, so that we can jump to the end of the statement
 * when we encounter a break statement. The same goes for continue statements,
 * which allow us to jump to the condition of the statement.
 * 
 * Therefore, we need to create a new compiler scope, so that the labels we
 * open do not conflict with the break and continue labels of the parent scope.
 */
bool parsepile_while(struct parser* parser, struct compiler* compiler) {
    t_compiler_label head;
    t_compiler_label end;
    struct compiler  subcompiler;
    bool             result;

    /*
     * Set up the return value.
     */
    result = false;

    /*
     * We create a new compiler scope, so that the labels we open
     * do not conflict with the labels of the parent scope.
     */
    compiler_create_sub(&subcompiler, compiler);

    /*
     * We then open two labels, one for the head of the while statement,
     * and one for the end of the while statement.
     * 
     * The head label is the place we jump to when `continue` is encountered.
     * The end label is the place we jump to when `break` is encountered.
     */
    head = compiler_open_continue_label(&subcompiler);
    end  = compiler_open_break_label(&subcompiler);

    /*
     * We then place the head label at the head of the while statement.
     */
    compiler_place_label(&subcompiler, head);

    /*
     * We then parsepile the condition of the while statement, which is
     * the expression between the parentheses.
     */
    if (parsepile_parenthesized_expression(parser, &subcompiler)) {
        /*
         * We jump to the end of the while statement if the condition
         * is false.
         */
        compiler_jump_if_not(&subcompiler, end);

        /*
         * We then parsepile the body of the while statement.
         */
        if (parsepile_instruction(parser, &subcompiler)) {
            /*
             * After that, we proceed to the head of the while statement.
             */
            compiler_jump(&subcompiler, head);

            /*
             * And place the end label at the end of the while statement.
             */
            compiler_place_label(&subcompiler, end);

            /*
             * Done! :)
             */
            result = true;
        }
    }

    /*
     * Close the labels we opened.
     * Cleanliness is next to godliness.
     */
    compiler_close_label(&subcompiler, head);
    compiler_close_label(&subcompiler, end);

    /*
     * Destroy the subcompiler. Boom.
     */
    compiler_destroy(&subcompiler);

    /*
     * Return the result. Over and out.
     */
    return result;
}


/*
 * Parse a do-while statement.
 *
 * A do-while statement is a conditional statement, which allows us to
 * execute a sequence of instructions as long as a condition is true.
 * 
 * A do-while statement looks like this:
 * 
 *     do {
 *       ...
 *     } while (x == 42);
 * 
 * This function parses a do-while statement, and returns true if the
 * statement was successfully parsed, and false otherwise.
 * 
 * The function works by first parsing the body of the do-while statement.
 * 
 * Since do-while statements are conditional statements, we need to make use
 * of the label facility of the compiler to implement the jumps.
 * 
 * A do-while statement translates to the following code:
 * 
 * L1:
 *     <body>
 *     <condition>
 *     jump_if L1
 * L2:
 * 
 * where L1, L2 are labels, and <condition> and <body> are the
 * instructions that make up the do-while statement.
 * 
 * The do-while statement is implemented by running the body of the statement,
 * and then evaluating the condition of the statement. If the condition is
 * true, we jump back to the body of the statement, and repeat the process.
 * 
 * The do-while statement is implemented using two labels, one for the
 * body of the statement, and one for the end of the statement.
 * 
 * The body of the statement is placed between the first and second labels.
 * 
 * The condition of the statement is placed between the second and third
 * labels.
 * 
 * Since do-while statements also allow us to break out of the loop, we need
 * to open a break label (L2), so that we can jump to the end of the statement
 * when we encounter a break statement. The label L1 is used to repeat the loop.
 * 
 * We do diverge from the usual implementation of do-while statements, in that
 * continue statements lead directly to the beginning of the loop, instead of
 * to the condition.
 * 
 * When compiling, we need to create a new compiler scope, so that the labels we
 * open do not conflict with the break and continue labels of the parent scope.
 * 
 * The function returns true if the do-while statement was successfully parsed,
 * and false otherwise.
 */
bool parsepile_do_while(struct parser* parser, struct compiler* compiler) {
    t_compiler_label head;
    t_compiler_label end;
    struct compiler  subcompiler;
    bool             result;

    /*
     * Set up the return value.
     */
    result = false;

    /*
     * We create a new compiler scope, so that the labels we open
     * do not conflict with the labels of the parent scope.
     */
    compiler_create_sub(&subcompiler, compiler);

    /*
     * We then open two labels, one for the head of the do-while statement,
     * and one for the end of the do-while statement.
     * 
     * The head label is the place we jump to when `continue` is encountered.
     * The end label is the place we jump to when `break` is encountered.
     */
    head = compiler_open_continue_label(&subcompiler);
    end  = compiler_open_break_label(&subcompiler);

    /*
     * Place the head label at the head of the do-while statement.
     */
    compiler_place_label(&subcompiler, head);

    /*
     * Parsepile the body of the do-while statement.
     *
     * This is one instruction, which could be a block statement.
     * But also other instructions are fine:
     * 
     *    do foo(); while (x == 42);
     */
    if (parsepile_instruction(parser, &subcompiler)) {

        /*
         * We then expect the `while` keyword.
         */
        if (parsepile_expect(parser, TOKEN_TYPE_KW_WHILE)) {

            /*
             * Parsepile the condition of the do-while statement,
             * which is the expression between the parentheses.
             */
            if (parsepile_parenthesized_expression(parser, &subcompiler)) {
                /*
                 * We then jump to the head of the do-while statement
                 * if the condition is true.
                 */
                compiler_jump_if(&subcompiler, head);

                /*
                 * After a do-while statement, there is always a semicolon.
                 */
                result = parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
            }
        }
    }

    /*
     * Place the end label at the end of the do-while statement.
     */
    compiler_place_label(&subcompiler, end);

    /*
     * Close the labels we opened.
     * Cleanliness is next to godliness.
     */
    compiler_close_label(&subcompiler, head);
    compiler_close_label(&subcompiler, end);

    /*
     * Destroy the subcompiler. Boom.
     */
    compiler_destroy(&subcompiler);

    /*
     * Return the result. Over and out.
     */
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
            compiler_add_var(&subcompiler, type, symbol);
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
                if (parsepile_expression(parser, &subcompiler)) {
                    iresult = parsepile_store_var(parser, &subcompiler, symbol);
                }
            }
        } else {
            iresult = parsepile_expression(parser, &subcompiler);
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

/*
 * Switch statements are rather tricky.
 * 
 * In a switch statement, control flow will jump to the first case that
 * matches, and then run all the code until it hits a break statement.
 * 
 * Since this parsepiler is a combination of a parser and a compiler, we
 * can't just read the switch statement, figure out the labels, and then
 * compile it. Instead, we have to compile it as we go.
 * 
 * This means that we will have to generate two different codepaths
 * at once (as we are unable to look ahead in the code):
 * 
 *     switch (x) {
 *                print("Zero ");
 *        case 1: print("One ");
 *        case 2: print("Two ");
 *                break;
 *        case 3: print("Three ");
 *                break;
 *        default: print("Default ");
 *     }
 * 
 * will be translated to this assembly code:
 * 
 *        print("Zero ");
 *    T0: <compare x to 1>
 *        <jump to T1 if not equal>
 *    L1: print("One ");
 *        <jump to L2>
 *    T1: <compare x to 2>
 *        <jump to T2 if not equal>
 *    L2: print("Two ");
 *        <jump to END>
 *    T2: <compare x to 3>
 *        <jump to T3 if not equal>
 *    L3: print("Three ");
 *        <jump to END>
 *    T3: print("Default ");
 *   END:
 * 
 * There are more complications to it, for example where to store the
 * value of the switch expression, and how to handle the default case.
 * 
 * Our algorithm puts the switch value on the stack, therefore the
 * compiler needs to write a lot of boilerplate and cleanup code.
 * 
 * It would also be possible to allocate a temporary variable for the
 * switch value, but the fact that there is no way to free them again
 * makes this a bad idea (for now).
 */
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
            /*
             * The head was parsed, and the value is on the stack.
             * We will now jump directly to the first decision point.
             */
            compiler_jump(&subcompiler, continuation);
            while (result && !parser_check(parser, TOKEN_TYPE_RCURLY)) {
                /*
                 * Process every statement in the switch block.
                 * 
                 * If the next one is a `case` statement, we have
                 * to switch to a different codepath. Remember that
                 * case statements will be ignored if they are not
                 * the one we jumped to, so we have to make sure
                 * that the test code is nicely embedded in the
                 * normal codepath.
                 * 
                 * Since we can't postpone the code generation, we
                 * have to jump around the `case` statement.
                 */
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
                     * originally pushed value stays on the stack, while also
                     * loading the value into the accumulator.
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

/*
 * Parsepile a `return` statement.
 *
 * Return statements look like this:
 * 
 *     return <expression>;
 * 
 * or
 * 
 *     return;
 * 
 * The first form returns the value of the expression, the second form
 * returns `nil`.
 * 
 * We typecheck the expression against the return type of the function.
 */
bool parsepile_return(struct parser* parser, struct compiler* compiler) {
    /*
     * The `return` statement is a bit special, because it can be used
     * in two different ways. We check for the semicolon first,
     * this way we can return `nil` if the semicolon is present.
     */
    if (parser_check(parser, TOKEN_TYPE_SEMICOLON)) {
        /*
         * Our statement is of the form `return;` which means we
         * return `nil`.
         */
        compiler_load_constant(compiler, any_nil());

        /*
         * We set the expression type to `void` to indicate that we
         * are returning `nil`.
         */
        parser_set_exprtype_to_void(parser);

        /*
         * Perform a typecheck and return.
         */
        return parsepile_return_with_typecheck(parser, compiler);
    } else if (parsepile_expression(parser, compiler)) {
        /*
         * Our statement is of the form `return <expression>;`.
         * The expression has been parsed and is in the accumulator.
         * We perform a typecheck and return.
         */
        return parsepile_return_with_typecheck(parser, compiler)
            && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
    }

    /*
     * We failed to parse the statement. Return false.
     */
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
                if (!parser_check(parser, TOKEN_TYPE_RPAREN)) {
                    result2 = false;
                    if (parse_fancy_vardecl(parser, &type, &name)) {
                        compiler_add_var(&subcompiler, type, name);
                        compiler_typecheck(&subcompiler, type);
                        compiler_store_var(&subcompiler, name);
                        result2 = parsepile_expect(parser, TOKEN_TYPE_RPAREN);
                    }
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
    } else if (parser_check(parser, TOKEN_TYPE_KW_FOREACH)) {
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

    result   = false;
    modifier = RAVEN_MODIFIER_NONE;

    while (true) {
        if (parse_modifier(parser, &modifier)) {
      
        } else if (parser_check(parser, TOKEN_TYPE_KW_OVERRIDE)) {
            /* TODO */
        } else if (parser_check(parser, TOKEN_TYPE_KW_DEPRECATED)) {
            /* TODO */
        } else {
            break;
        }
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
 * Handle the second part of an 'inherit' statement.
 *
 * This function is a utility function that performs
 * all of the lookup and dereferencing operations
 * associated with an 'inherited' statement.
 *
 * It is used in two places:
 *   - After an 'inherit' token.
 *   - After a 'class' token (short form).
 */
bool parsepile_inheritance_impl(struct parser*    parser,
                                struct blueprint* into,
                                bool*             has_inheritance) {
    struct blueprint*  bp;
    bool               result;

    result = false;

    if (has_inheritance != NULL)
        *has_inheritance = false;

    /* 'inherit;' inherits from nothing - needed for "/secure/base" itself */
    if (parser_check(parser, TOKEN_TYPE_SEMICOLON)) {
        result = true;
    } else if (parsepile_expect_noadvance(parser, TOKEN_TYPE_STRING)) {
        if (has_inheritance != NULL)
            *has_inheritance = true;
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
        result = parsepile_inheritance_impl(parser, into, &has_inheritance);
    } else {
        bp = raven_get_blueprint(parser->raven, "/secure/base", true);
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
    struct stringbuilder  sb;
    struct reader         reader;
    struct parser         new_parser;
    struct fs_pather      pather;
    bool                  result;

    result = false;
    if (parsepile_expect_noadvance(parser, TOKEN_TYPE_STRING)) {
        fs_pather_create(&pather);
        fs_pather_cd(&pather, blueprint_virt_path(into));
        fs_pather_cd(&pather, "..");
        fs_pather_cd(&pather, parser_as_cstr(parser));

        parser_advance(parser);

        stringbuilder_create(&sb);
        {
            if (fs_read(raven_fs(parser_raven(parser)), fs_pather_get_const(&pather), &sb)) {
                reader_create(&reader, stringbuilder_get_const(&sb));
                parser_create(&new_parser, parser_raven(parser), &reader, parser_log(parser));
                result = parsepile_file_impl(&new_parser, into, false, TOKEN_TYPE_EOF);
                parser_destroy(&new_parser);
                reader_destroy(&reader);
            } else {
                parser_error(parser, "File not found!");
            }
        }
        stringbuilder_destroy(&sb);
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
    struct blueprint*  parent;
    struct object*     object;
    bool               preresult;
    bool               result;
    bool               postcheck;
    bool               has_inheritance;

    preresult = false;
    result    = false;
    postcheck = false;

    if (expect_symbol(parser, &name)) {
        blue = blueprint_new(parser_raven(parser), NULL);
        if (blue != NULL) {
            /*
             * There are two possible notations.
             *
             * Either the explicit form:
             *
             *     class foo {
             *       inherit "/std/thing";
             *
             *       // ...
             *     };
             *
             * Or the short form:
             *
             *     class foo "/std/thing";
             *
             */
            if (parser_check(parser, TOKEN_TYPE_LCURLY)) {
                postcheck = true;
                if (parsepile_file_impl(parser, blue, true, TOKEN_TYPE_RCURLY)) {
                    preresult = true;
                }
            } else {
                if (parsepile_inheritance_impl(parser, blue, &has_inheritance)) {
                    if (!has_inheritance) {
                        parent = raven_get_blueprint(parser->raven, "/secure/base", true);
                        if (parent != NULL && blueprint_inherit(blue, parent)) {
                            preresult = true;
                        }
                    } else {
                        preresult = true;
                    }
                }
            }

            if (preresult) {
                /*
                 * TODO, FIXME, XXX: Check for NULL return value!
                 */
                object = blueprint_instantiate(blue, parser_raven(parser));
                types  = raven_types(parser_raven(parser));
                blueprint_add_var(into, typeset_type_object(types), name);
                compiler_load_constant(compiler, any_from_ptr(object));
                compiler_op(compiler, RAVEN_OP_DEREF);
                compiler_store_var(compiler, name);
                if (postcheck) {
                    result = parsepile_expect(parser, TOKEN_TYPE_RCURLY)
                        && parsepile_expect(parser, TOKEN_TYPE_SEMICOLON);
                } else {
                    result = true;
                }
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

