/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_LANG_PARSER_H
#define RAVEN_LANG_PARSER_H

#include "../../defs.h"
#include "../../raven/raven.h"
#include "../../util/log.h"
#include "../../util/utf8.h"
#include "../core/type.h"

#include "reader.h"

enum token_type {
    TOKEN_TYPE_ERROR,
    TOKEN_TYPE_EOF,

    TOKEN_TYPE_IDENT,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_CHAR,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_SYMBOL,

    TOKEN_TYPE_LPAREN,
    TOKEN_TYPE_RPAREN,
    TOKEN_TYPE_LBRACK,
    TOKEN_TYPE_RBRACK,
    TOKEN_TYPE_LCURLY,
    TOKEN_TYPE_RCURLY,
    TOKEN_TYPE_DOT,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_SCOPE,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_ELLIPSIS,
    TOKEN_TYPE_RANGE,

    TOKEN_TYPE_EQUALS,
    TOKEN_TYPE_NOT_EQUALS,
    TOKEN_TYPE_LESS,
    TOKEN_TYPE_LEQ,
    TOKEN_TYPE_GREATER,
    TOKEN_TYPE_GEQ,

    TOKEN_TYPE_OR,
    TOKEN_TYPE_AND,
    TOKEN_TYPE_NOT,

    TOKEN_TYPE_ASSIGNMENT,
    TOKEN_TYPE_ARROW,
    TOKEN_TYPE_PARROW,
    TOKEN_TYPE_AMPERSAND,
    TOKEN_TYPE_PIPE,
    TOKEN_TYPE_LEFTSHIFT,
    TOKEN_TYPE_RIGHTSHIFT,
    TOKEN_TYPE_QUESTIONQUESTION,
    TOKEN_TYPE_QUESTION,

    TOKEN_TYPE_INC,
    TOKEN_TYPE_DEC,

    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_SLASH,
    TOKEN_TYPE_PERCENT,

    TOKEN_TYPE_PLUS_ASSIGNMENT,
    TOKEN_TYPE_MINUS_ASSIGNMENT,
    TOKEN_TYPE_STAR_ASSIGNMENT,
    TOKEN_TYPE_SLASH_ASSIGNMENT,
    TOKEN_TYPE_PERCENT_ASSIGNMENT,

    TOKEN_TYPE_KW_INCLUDE,
    TOKEN_TYPE_KW_INHERIT,
    TOKEN_TYPE_KW_PRIVATE,
    TOKEN_TYPE_KW_PROTECTED,
    TOKEN_TYPE_KW_PUBLIC,
    TOKEN_TYPE_KW_OVERRIDE,
    TOKEN_TYPE_KW_DEPRECATED,
    TOKEN_TYPE_KW_NOSAVE,
    TOKEN_TYPE_KW_NEW,
    TOKEN_TYPE_KW_THIS,
    TOKEN_TYPE_KW_NIL,
    TOKEN_TYPE_KW_TRUE,
    TOKEN_TYPE_KW_FALSE,
    TOKEN_TYPE_KW_SIZEOF,
    TOKEN_TYPE_KW_IS,
    TOKEN_TYPE_KW_CLASS,
    TOKEN_TYPE_KW_VOID,
    TOKEN_TYPE_KW_CHAR,
    TOKEN_TYPE_KW_INT,
    TOKEN_TYPE_KW_BOOL,
    TOKEN_TYPE_KW_OBJECT,
    TOKEN_TYPE_KW_STRING,
    TOKEN_TYPE_KW_SYMBOL,
    TOKEN_TYPE_KW_MAPPING,
    TOKEN_TYPE_KW_ANY,
    TOKEN_TYPE_KW_MIXED,
    TOKEN_TYPE_KW_AUTO,
    TOKEN_TYPE_KW_OPERATOR,
    TOKEN_TYPE_KW_LET,
    TOKEN_TYPE_KW_IF,
    TOKEN_TYPE_KW_ELSE,
    TOKEN_TYPE_KW_WHILE,
    TOKEN_TYPE_KW_DO,
    TOKEN_TYPE_KW_FOR,
    TOKEN_TYPE_KW_FOREACH,
    TOKEN_TYPE_KW_SWITCH,
    TOKEN_TYPE_KW_CASE,
    TOKEN_TYPE_KW_DEFAULT,
    TOKEN_TYPE_KW_BREAK,
    TOKEN_TYPE_KW_CONTINUE,
    TOKEN_TYPE_KW_RETURN,
    TOKEN_TYPE_KW_TRY,
    TOKEN_TYPE_KW_CATCH
};

const char* token_type_name(enum token_type type);


#define PARSER_BUFFER_SIZE 1024*16

struct parser {
    struct raven*   raven;
    t_reader*       reader;
    char*           file_name;
    struct file_pos file_pos;
    struct log*     log;
    enum token_type token_type;
    struct type*    exprtype;
    struct type*    returntype;
    int             integer;
    unsigned int    buffer_fill;
    char            buffer[PARSER_BUFFER_SIZE];
};

void parser_create(struct parser* parser,
                   struct raven*  raven,
                   t_reader*      reader,
                   struct log*    log);
void parser_destroy(struct parser* parser);

const char* parser_file_name(struct parser* parser);
void        parser_set_file_name(struct parser* parser, const char* name);

bool parser_is(struct parser* parser, enum token_type type);
bool parser_check(struct parser* parser, enum token_type type);
void parser_advance(struct parser* parser);

bool parser_check_cstr(struct parser* parser, const char* str);

struct symbol* parser_as_symbol(struct parser* parser);
struct blueprint* parser_as_blueprint(struct parser* parser);
struct blueprint* parser_as_relative_blueprint(struct parser* parser,
                                               struct blueprint* from);
int parser_as_int(struct parser* parser);
struct string* parser_as_string(struct parser* parser);
struct symbol* parser_as_symbol(struct parser* parser);
raven_rune_t parser_as_char(struct parser* parser);

unsigned int parser_unary_prec(struct parser* parser);
unsigned int parser_binary_prec(struct parser* parser);

void parser_set_returntype(struct parser* parser, struct type* type);
void parser_reset_returntype(struct parser* parser);

void parser_set_exprtype(struct parser* parser, struct type* type);
void parser_set_exprtype_to_void(struct parser* parser);
void parser_set_exprtype_to_any(struct parser* parser);
void parser_set_exprtype_to_bool(struct parser* parser);
void parser_set_exprtype_to_int(struct parser* parser);
void parser_set_exprtype_to_char(struct parser* parser);
void parser_set_exprtype_to_string(struct parser* parser);
void parser_set_exprtype_to_symbol(struct parser* parser);
void parser_set_exprtype_to_object(struct parser* parser);
void parser_set_exprtype_to_array(struct parser* parser);
void parser_set_exprtype_to_mapping(struct parser* parser);
void parser_reset_exprtype(struct parser* parser);

static inline struct raven* parser_raven(struct parser* parser) {
    return parser->raven;
}

static inline struct log* parser_log(struct parser* parser) {
    return parser->log;
}

static inline const char* parser_src(struct parser* parser) {
    return reader_src(parser->reader);
}

static inline unsigned int parser_line(struct parser* parser) {
    return parser->file_pos.line;
}

static inline unsigned int parser_caret(struct parser* parser) {
    return parser->file_pos.caret;
}

static inline struct type* parser_get_exprtype(struct parser* parser) {
    return parser->exprtype;
}

static inline struct type* parser_get_returntype(struct parser* parser) {
    return parser->returntype;
}

static inline const char* parser_as_cstr(struct parser* parser) {
    return parser->buffer;
}

#endif
