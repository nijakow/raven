/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../core/objects/string.h"
#include "../core/blueprint.h"

#include "parser.h"

static const char* IDENT_CHARS =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$#";


const char* token_type_name(enum token_type type) {
    switch (type) {
    case TOKEN_TYPE_EOF: return "EOF";
    case TOKEN_TYPE_IDENT: return "IDENTIFIER";
    case TOKEN_TYPE_INT: return "INT";
    case TOKEN_TYPE_CHAR: return "CHAR";
    case TOKEN_TYPE_STRING: return "STRING";
    case TOKEN_TYPE_LPAREN: return "LPAREN";
    case TOKEN_TYPE_RPAREN: return "RPAREN";
    case TOKEN_TYPE_LBRACK: return "LBRACK";
    case TOKEN_TYPE_RBRACK: return "RBRACK";
    case TOKEN_TYPE_LCURLY: return "LCURLY";
    case TOKEN_TYPE_RCURLY: return "RCURLY";
    case TOKEN_TYPE_DOT: return "DOT";
    case TOKEN_TYPE_COMMA: return "COMMA";
    case TOKEN_TYPE_SCOPE: return "SCOPE";
    case TOKEN_TYPE_COLON: return "COLON";
    case TOKEN_TYPE_SEMICOLON: return "SEMICOLON";
    case TOKEN_TYPE_ELLIPSIS: return "ELLIPSIS";
    case TOKEN_TYPE_EQUALS: return "EQUALS";
    case TOKEN_TYPE_NOT_EQUALS: return "NOT_EQUALS";
    case TOKEN_TYPE_LESS: return "LESS";
    case TOKEN_TYPE_LEQ: return "LEQ";
    case TOKEN_TYPE_GREATER: return "GREATER";
    case TOKEN_TYPE_GEQ: return "GEQ";
    case TOKEN_TYPE_OR: return "OR";
    case TOKEN_TYPE_AND: return "AND";
    case TOKEN_TYPE_NOT: return "NOT";
    case TOKEN_TYPE_ASSIGNMENT: return "ASSIGNMENT";
    case TOKEN_TYPE_ARROW: return "ARROW";
    case TOKEN_TYPE_AMPERSAND: return "AMPERSAND";
    case TOKEN_TYPE_QUESTION: return "QUESTION";
    case TOKEN_TYPE_INC: return "INC";
    case TOKEN_TYPE_DEC: return "DEC";
    case TOKEN_TYPE_PLUS: return "PLUS";
    case TOKEN_TYPE_MINUS: return "MINUS";
    case TOKEN_TYPE_STAR: return "STAR";
    case TOKEN_TYPE_SLASH: return "SLASH";
    case TOKEN_TYPE_PERCENT: return "PERCENT";
    case TOKEN_TYPE_PLUS_ASSIGNMENT: return "PLUS_ASSIGNMENT";
    case TOKEN_TYPE_MINUS_ASSIGNMENT: return "MINUS_ASSIGNMENT";
    case TOKEN_TYPE_STAR_ASSIGNMENT: return "STAR_ASSIGNMENT";
    case TOKEN_TYPE_SLASH_ASSIGNMENT: return "SLASH_ASSIGNMENT";
    case TOKEN_TYPE_PERCENT_ASSIGNMENT: return "PERCENT_ASSIGNMENT";
    case TOKEN_TYPE_KW_INCLUDE: return "KW_INCLUDE";
    case TOKEN_TYPE_KW_INHERIT: return "KW_INHERIT";
    case TOKEN_TYPE_KW_PRIVATE: return "KW_PRIVATE";
    case TOKEN_TYPE_KW_PROTECTED: return "KW_PROTECTED";
    case TOKEN_TYPE_KW_PUBLIC: return "KW_PUBLIC";
    case TOKEN_TYPE_KW_OVERRIDE: return "KW_OVERRIDE";
    case TOKEN_TYPE_KW_DEPRECATED: return "KW_DEPRECATED";
    case TOKEN_TYPE_KW_NEW: return "KW_NEW";
    case TOKEN_TYPE_KW_THIS: return "KW_THIS";
    case TOKEN_TYPE_KW_NIL: return "KW_NIL";
    case TOKEN_TYPE_KW_TRUE: return "KW_TRUE";
    case TOKEN_TYPE_KW_FALSE: return "KW_FALSE";
    case TOKEN_TYPE_KW_SIZEOF: return "KW_SIZEOF";
    case TOKEN_TYPE_KW_IS: return "KW_IS";
    case TOKEN_TYPE_KW_CLASS: return "KW_CLASS";
    case TOKEN_TYPE_KW_VOID: return "KW_VOID";
    case TOKEN_TYPE_KW_CHAR: return "KW_CHAR";
    case TOKEN_TYPE_KW_INT: return "KW_INT";
    case TOKEN_TYPE_KW_BOOL: return "KW_BOOL";
    case TOKEN_TYPE_KW_OBJECT: return "KW_OBJECT";
    case TOKEN_TYPE_KW_STRING: return "KW_STRING";
    case TOKEN_TYPE_KW_SYMBOL: return "KW_SYMBOL";
    case TOKEN_TYPE_KW_MAPPING: return "KW_MAPPING";
    case TOKEN_TYPE_KW_ANY: return "KW_ANY";
    case TOKEN_TYPE_KW_MIXED: return "KW_MIXED";
    case TOKEN_TYPE_KW_AUTO: return "KW_AUTO";
    case TOKEN_TYPE_KW_LET: return "KW_LET";
    case TOKEN_TYPE_KW_IF: return "KW_IF";
    case TOKEN_TYPE_KW_ELSE: return "KW_ELSE";
    case TOKEN_TYPE_KW_WHILE: return "KW_WHILE";
    case TOKEN_TYPE_KW_DO: return "KW_DO";
    case TOKEN_TYPE_KW_FOR: return "KW_FOR";
    case TOKEN_TYPE_KW_SWITCH: return "KW_SWITCH";
    case TOKEN_TYPE_KW_CASE: return "KW_CASE";
    case TOKEN_TYPE_KW_DEFAULT: return "KW_DEFAULT";
    case TOKEN_TYPE_KW_BREAK: return "KW_BREAK";
    case TOKEN_TYPE_KW_CONTINUE: return "KW_CONTINUE";
    case TOKEN_TYPE_KW_RETURN: return "KW_RETURN";
    case TOKEN_TYPE_KW_TRY: return "KW_TRY";
    case TOKEN_TYPE_KW_CATCH: return "KW_CATCH";
    default: return "UNKNOWN";
    }
}


void parser_create(struct parser* parser,
                   struct raven*  raven,
                   t_reader*      reader,
                   struct log*    log) {
    parser->raven          = raven;
    parser->reader         = reader;
    parser->file_name      = NULL;
    parser->file_pos.line  = 0;
    parser->file_pos.caret = 0;
    parser->log            = log;
    parser->token_type     = TOKEN_TYPE_EOF;
    parser->buffer_fill    = 0;
    parser_reset_exprtype(parser);
    parser_reset_returntype(parser);
    parser_advance(parser);
}

void parser_destroy(struct parser* parser) {
}

const char* parser_file_name(struct parser* parser) {
    return parser->file_name;
}

void parser_set_file_name(struct parser* parser, const char* name) {
    parser->file_name = (char*) name;
}


static void parser_set_type(struct parser* parser, enum token_type type) {
    parser->token_type = type;
}

static void parser_buffer_clear(struct parser* parser) {
    parser->buffer_fill = 0;
    parser->buffer[0]   = '\0';
}

static bool parser_buffer_is_empty(struct parser* parser) {
    return parser->buffer_fill == 0;
}

static void parser_buffer_append(struct parser* parser, char c) {
    if (parser->buffer_fill < sizeof(parser->buffer) - 1) {
        parser->buffer[parser->buffer_fill++] = c;
        parser->buffer[parser->buffer_fill]   = '\0';
    }
}

static void parser_buffer_append_rune(struct parser* parser, raven_rune_t rune) {
    char utf8[5];
    size_t len;
    size_t index;
    
    len = utf8_encode(rune, utf8);
    for (index = 0; index < len; index++)
        parser_buffer_append(parser, utf8[index]);
}

static bool parser_buffer_is(struct parser* parser, const char* str) {
    unsigned int i;

    for (i = 0; i < parser->buffer_fill; i++) {
        if (parser->buffer[i] != str[i] || str[i] == '\0')
            return false;
    }

    return str[i] == '\0';
}

bool parser_is(struct parser* parser, enum token_type type) {
    return parser->token_type == type;
}

bool parser_check(struct parser* parser, enum token_type type) {
    if (parser_is(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

raven_rune_t parser_read_escaped_char(struct parser* parser) {
    raven_rune_t  c;

    if (reader_check(parser->reader, '\\')) {
        c = reader_advance_rune(parser->reader);
        switch (c) {
        case 't': return '\t';
        case 'r': return '\r';
        case 'n': return '\n';
        case 'e': return '\033';
        case '{': return '\02';
        case '}': return '\03';
        default: return c;
        }
    } else {
        return reader_advance_rune(parser->reader);
    }
}

void parser_read_until(struct parser* parser, const char* stop) {
    raven_rune_t  rune;
    size_t        index;
    size_t        size;
    char          utf8[5];

    while (reader_has(parser->reader) && !reader_checks(parser->reader, stop)) {
        rune = parser_read_escaped_char(parser);
        size = utf8_encode(rune, utf8);
        for (index = 0; index < size; index++)
            parser_buffer_append(parser, utf8[index]);
    }
}

void parser_raw_read_until(struct parser* parser, const char* stop) {
    while (reader_has(parser->reader) && !reader_checks(parser->reader, stop)) {
        parser_buffer_append(parser, reader_advance(parser->reader));
    }
}

bool parser_isdigit(char c) {
    return (c >= '0' && c <= '9');
}

void parser_read_int(struct parser* parser) {
    parser->integer = 0;
    while (reader_has(parser->reader)
           && parser_isdigit(reader_peek(parser->reader))) {
        parser->integer *= 10;
        parser->integer += reader_advance(parser->reader) - '0';
    }
}

void parser_read_string(struct parser* parser, const char* stop) {
    do {
        parser_read_until(parser, stop);
        reader_skip_whitespace(parser->reader);
    } while (reader_has(parser->reader) && reader_checks(parser->reader, stop));
}

void parser_read_raw_string(struct parser* parser) {
    do {
        parser_raw_read_until(parser, "!*/");
        reader_skip_whitespace(parser->reader);
    } while (reader_has(parser->reader)
             && reader_checks(parser->reader, "!*/"));
}

void parser_read_character(struct parser* parser) {
    raven_rune_t c;

    c = parser_read_escaped_char(parser);
    parser_buffer_append_rune(parser, c);
    /* TODO: This must be '\''! Otherwise: Error! */
    reader_advance(parser->reader);
}

void parser_read_symbol(struct parser* parser) {
    char c;

    while (reader_peekn(parser->reader, &c, IDENT_CHARS)) {
        parser_buffer_append(parser, c);
    }
}

void parser_advance(struct parser* parser) {
    parser_buffer_clear(parser);

    reader_skip_whitespace(parser->reader);

    parser->file_pos = reader_file_pos(parser->reader);

    if (!reader_has(parser->reader)) {
        parser_set_type(parser, TOKEN_TYPE_EOF);
    } else if (reader_checks(parser->reader, "/*!")) {
        parser_set_type(parser, TOKEN_TYPE_STRING);
        parser_read_raw_string(parser);
    } else if (reader_checks(parser->reader, "/*")) {
        parser_read_until(parser, "*/");
        parser_advance(parser);
        return;
    } else if (reader_checks(parser->reader, "//")) {
        parser_read_until(parser, "\n");
        parser_advance(parser);
        return;
    } else if (reader_checks(parser->reader, "(")) {
        parser_set_type(parser, TOKEN_TYPE_LPAREN);
    } else if (reader_checks(parser->reader, ")")) {
        parser_set_type(parser, TOKEN_TYPE_RPAREN);
    } else if (reader_checks(parser->reader, "[")) {
        parser_set_type(parser, TOKEN_TYPE_LBRACK);
    } else if (reader_checks(parser->reader, "]")) {
        parser_set_type(parser, TOKEN_TYPE_RBRACK);
    } else if (reader_checks(parser->reader, "{")) {
        parser_set_type(parser, TOKEN_TYPE_LCURLY);
    } else if (reader_checks(parser->reader, "}")) {
        parser_set_type(parser, TOKEN_TYPE_RCURLY);
    } else if (reader_checks(parser->reader, "...")) {
        parser_set_type(parser, TOKEN_TYPE_ELLIPSIS);
    } else if (reader_checks(parser->reader, ".")) {
        parser_set_type(parser, TOKEN_TYPE_DOT);
    } else if (reader_checks(parser->reader, ",")) {
        parser_set_type(parser, TOKEN_TYPE_COMMA);
    } else if (reader_checks(parser->reader, "::")) {
        parser_set_type(parser, TOKEN_TYPE_SCOPE);
    } else if (reader_checks(parser->reader, ":")) {
        parser_set_type(parser, TOKEN_TYPE_COLON);
    } else if (reader_checks(parser->reader, ";")) {
        parser_set_type(parser, TOKEN_TYPE_SEMICOLON);
    } else if (reader_checks(parser->reader, "==")) {
        parser_set_type(parser, TOKEN_TYPE_EQUALS);
    } else if (reader_checks(parser->reader, "!=")) {
        parser_set_type(parser, TOKEN_TYPE_NOT_EQUALS);
    } else if (reader_checks(parser->reader, "<=")) {
        parser_set_type(parser, TOKEN_TYPE_LEQ);
    } else if (reader_checks(parser->reader, "<")) {
        parser_set_type(parser, TOKEN_TYPE_LESS);
    } else if (reader_checks(parser->reader, ">=")) {
        parser_set_type(parser, TOKEN_TYPE_GEQ);
    } else if (reader_checks(parser->reader, ">")) {
        parser_set_type(parser, TOKEN_TYPE_GREATER);
    } else if (reader_checks(parser->reader, "||")) {
        parser_set_type(parser, TOKEN_TYPE_OR);
    } else if (reader_checks(parser->reader, "&&")) {
        parser_set_type(parser, TOKEN_TYPE_AND);
    } else if (reader_checks(parser->reader, "!")) {
        parser_set_type(parser, TOKEN_TYPE_NOT);
    } else if (reader_checks(parser->reader, "=")) {
        parser_set_type(parser, TOKEN_TYPE_ASSIGNMENT);
    } else if (reader_checks(parser->reader, "->")) {
        parser_set_type(parser, TOKEN_TYPE_ARROW);
    } else if (reader_checks(parser->reader, "&")) {
        parser_set_type(parser, TOKEN_TYPE_AMPERSAND);
    } else if (reader_checks(parser->reader, "?")) {
        parser_set_type(parser, TOKEN_TYPE_QUESTION);
    } else if (reader_checks(parser->reader, "+=")) {
        parser_set_type(parser, TOKEN_TYPE_PLUS_ASSIGNMENT);
    } else if (reader_checks(parser->reader, "-=")) {
        parser_set_type(parser, TOKEN_TYPE_MINUS_ASSIGNMENT);
    } else if (reader_checks(parser->reader, "*=")) {
        parser_set_type(parser, TOKEN_TYPE_STAR_ASSIGNMENT);
    } else if (reader_checks(parser->reader, "/=")) {
        parser_set_type(parser, TOKEN_TYPE_SLASH_ASSIGNMENT);
    } else if (reader_checks(parser->reader, "%=")) {
        parser_set_type(parser, TOKEN_TYPE_PERCENT_ASSIGNMENT);
    } else if (reader_checks(parser->reader, "++")) {
        parser_set_type(parser, TOKEN_TYPE_INC);
    } else if (reader_checks(parser->reader, "--")) {
        parser_set_type(parser, TOKEN_TYPE_DEC);
    } else if (reader_checks(parser->reader, "+")) {
        parser_set_type(parser, TOKEN_TYPE_PLUS);
    } else if (reader_checks(parser->reader, "-")) {
        parser_set_type(parser, TOKEN_TYPE_MINUS);
    } else if (reader_checks(parser->reader, "*")) {
        parser_set_type(parser, TOKEN_TYPE_STAR);
    } else if (reader_checks(parser->reader, "/")) {
        parser_set_type(parser, TOKEN_TYPE_SLASH);
    } else if (reader_checks(parser->reader, "%")) {
        parser_set_type(parser, TOKEN_TYPE_PERCENT);
    } else if (reader_checks(parser->reader, "\"")) {
        parser_set_type(parser, TOKEN_TYPE_STRING);
        parser_read_string(parser, "\"");
    } else if (reader_checks(parser->reader, "\'")) {
        parser_set_type(parser, TOKEN_TYPE_CHAR);
        parser_read_character(parser);
    } else if (reader_checks(parser->reader, "#\'")) {
        parser_set_type(parser, TOKEN_TYPE_SYMBOL);
        parser_read_string(parser, "\'");
    } else if (reader_checks(parser->reader, "#:")) {
        parser_set_type(parser, TOKEN_TYPE_SYMBOL);
        parser_read_symbol(parser);
    } else if (parser_isdigit(reader_peek(parser->reader))) {
        parser_set_type(parser, TOKEN_TYPE_INT);
        parser_read_int(parser);
    } else {
        parser_read_symbol(parser);

        if (parser_buffer_is_empty(parser)) {
            parser_set_type(parser, TOKEN_TYPE_EOF);
        } else if (parser_buffer_is(parser, "#include")) {
            parser_set_type(parser, TOKEN_TYPE_KW_INCLUDE);
        } else if (parser_buffer_is(parser, "inherit")) {
            parser_set_type(parser, TOKEN_TYPE_KW_INHERIT);
        } else if (parser_buffer_is(parser, "private")) {
            parser_set_type(parser, TOKEN_TYPE_KW_PRIVATE);
        } else if (parser_buffer_is(parser, "protected")) {
            parser_set_type(parser, TOKEN_TYPE_KW_PROTECTED);
        } else if (parser_buffer_is(parser, "public")) {
            parser_set_type(parser, TOKEN_TYPE_KW_PUBLIC);
        } else if (parser_buffer_is(parser, "override")) {
            parser_set_type(parser, TOKEN_TYPE_KW_OVERRIDE);
        } else if (parser_buffer_is(parser, "deprecated")) {
            parser_set_type(parser, TOKEN_TYPE_KW_DEPRECATED);
        } else if (parser_buffer_is(parser, "new")) {
            parser_set_type(parser, TOKEN_TYPE_KW_NEW);
        } else if (parser_buffer_is(parser, "this")) {
            parser_set_type(parser, TOKEN_TYPE_KW_THIS);
        } else if (parser_buffer_is(parser, "nil")) {
            parser_set_type(parser, TOKEN_TYPE_KW_NIL);
        } else if (parser_buffer_is(parser, "true")) {
            parser_set_type(parser, TOKEN_TYPE_KW_TRUE);
        } else if (parser_buffer_is(parser, "false")) {
            parser_set_type(parser, TOKEN_TYPE_KW_FALSE);
        } else if (parser_buffer_is(parser, "sizeof")) {
            parser_set_type(parser, TOKEN_TYPE_KW_SIZEOF);
        } else if (parser_buffer_is(parser, "is")) {
            parser_set_type(parser, TOKEN_TYPE_KW_IS);
        } else if (parser_buffer_is(parser, "class")) {
            parser_set_type(parser, TOKEN_TYPE_KW_CLASS);
        } else if (parser_buffer_is(parser, "void")) {
            parser_set_type(parser, TOKEN_TYPE_KW_VOID);
        } else if (parser_buffer_is(parser, "char")) {
            parser_set_type(parser, TOKEN_TYPE_KW_CHAR);
        } else if (parser_buffer_is(parser, "int")) {
            parser_set_type(parser, TOKEN_TYPE_KW_INT);
        } else if (parser_buffer_is(parser, "bool")) {
            parser_set_type(parser, TOKEN_TYPE_KW_BOOL);
        } else if (parser_buffer_is(parser, "object")) {
            parser_set_type(parser, TOKEN_TYPE_KW_OBJECT);
        } else if (parser_buffer_is(parser, "string")) {
            parser_set_type(parser, TOKEN_TYPE_KW_STRING);
        } else if (parser_buffer_is(parser, "symbol")) {
            parser_set_type(parser, TOKEN_TYPE_KW_SYMBOL);
        } else if (parser_buffer_is(parser, "mapping")) {
            parser_set_type(parser, TOKEN_TYPE_KW_MAPPING);
        } else if (parser_buffer_is(parser, "any")) {
            parser_set_type(parser, TOKEN_TYPE_KW_ANY);
        } else if (parser_buffer_is(parser, "mixed")) {
            parser_set_type(parser, TOKEN_TYPE_KW_MIXED);
        } else if (parser_buffer_is(parser, "auto")) {
            parser_set_type(parser, TOKEN_TYPE_KW_AUTO);
        } else if (parser_buffer_is(parser, "let")) {
            parser_set_type(parser, TOKEN_TYPE_KW_LET);
        } else if (parser_buffer_is(parser, "if")) {
            parser_set_type(parser, TOKEN_TYPE_KW_IF);
        } else if (parser_buffer_is(parser, "else")) {
            parser_set_type(parser, TOKEN_TYPE_KW_ELSE);
        } else if (parser_buffer_is(parser, "while")) {
            parser_set_type(parser, TOKEN_TYPE_KW_WHILE);
        } else if (parser_buffer_is(parser, "do")) {
            parser_set_type(parser, TOKEN_TYPE_KW_DO);
        } else if (parser_buffer_is(parser, "for")) {
            parser_set_type(parser, TOKEN_TYPE_KW_FOR);
        } else if (parser_buffer_is(parser, "foreach")) {
            parser_set_type(parser, TOKEN_TYPE_KW_FOREACH);
        } else if (parser_buffer_is(parser, "switch")) {
            parser_set_type(parser, TOKEN_TYPE_KW_SWITCH);
        } else if (parser_buffer_is(parser, "case")) {
            parser_set_type(parser, TOKEN_TYPE_KW_CASE);
        } else if (parser_buffer_is(parser, "default")) {
            parser_set_type(parser, TOKEN_TYPE_KW_DEFAULT);
        } else if (parser_buffer_is(parser, "break")) {
            parser_set_type(parser, TOKEN_TYPE_KW_BREAK);
        } else if (parser_buffer_is(parser, "continue")) {
            parser_set_type(parser, TOKEN_TYPE_KW_CONTINUE);
        } else if (parser_buffer_is(parser, "return")) {
            parser_set_type(parser, TOKEN_TYPE_KW_RETURN);
        } else if (parser_buffer_is(parser, "try")) {
            parser_set_type(parser, TOKEN_TYPE_KW_TRY);
        } else if (parser_buffer_is(parser, "catch")) {
            parser_set_type(parser, TOKEN_TYPE_KW_CATCH);
        } else {
            parser_set_type(parser, TOKEN_TYPE_IDENT);
        }
    }
}

struct symbol* parser_as_symbol(struct parser* parser) {
    if (parser_buffer_is_empty(parser))
        return false;
    return raven_find_symbol(parser->raven, parser->buffer);
}

struct blueprint* parser_as_blueprint(struct parser* parser) {
    if (parser_buffer_is_empty(parser))
        return false;
    return raven_get_blueprint(parser->raven, parser->buffer);
}

struct blueprint* parser_as_relative_blueprint(struct parser* parser,
                                               struct blueprint* from) {
    if (parser_buffer_is_empty(parser))
        return false;
    return blueprint_load_relative(from, parser->buffer);
}

int parser_as_int(struct parser* parser) {
    return parser->integer;
}

struct string* parser_as_string(struct parser* parser) {
    return string_new(parser->raven, parser->buffer);
}

struct symbol* parser_set_exprtype_as_symbol(struct parser* parser) {
    return raven_find_symbol(parser->raven, parser->buffer);
}

raven_rune_t parser_as_char(struct parser* parser) {
    size_t        amount;

    return utf8_decode(parser->buffer, &amount);
}

void parser_set_returntype(struct parser* parser, struct type* type) {
    parser->returntype = type;
}

void parser_reset_returntype(struct parser* parser) {
    parser->returntype = NULL;
}

void parser_set_exprtype(struct parser* parser, struct type* type) {
    parser->exprtype = type;
}

void parser_set_exprtype_to_void(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_void(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_any(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_any(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_bool(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_bool(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_int(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_int(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_char(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_char(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_string(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_string(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_symbol(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_symbol(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_object(struct parser* parser) {
    parser_set_exprtype(parser,
                        typeset_type_object(raven_types(parser_raven(parser))));
}

void parser_set_exprtype_to_array(struct parser* parser) {
    parser_set_exprtype_to_any(parser); /* TODO */
}

void parser_set_exprtype_to_mapping(struct parser* parser) {
    parser_set_exprtype_to_any(parser); /* TODO */
}

void parser_reset_exprtype(struct parser* parser) {
    parser_set_exprtype_to_any(parser);
}
