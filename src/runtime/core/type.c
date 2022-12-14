/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../raven/raven.h"

#include "../core/objects/string.h"

#include "type.h"


static bool type_check_func_all(struct type* type, any value) {
    return true;
}

static bool type_cast_func_all(struct type* type, any* value) {
    return true;
}

static bool type_check_func_any(struct type* type, any value) {
    return type_check_func_all(type, value);
}

static bool type_cast_func_any(struct type* type, any* value) {
    return type_cast_func_all(type, value);
}

static bool type_check_func_nil(struct type* type, any value) {
    return any_is_nil(value);
}

static bool type_cast_func_nil(struct type* type, any* value) {
    *value = any_nil();
    return true;
}

static bool type_check_func_void(struct type* type, any value) {
    return type_check_func_nil(type, value);
}

static bool type_cast_func_void(struct type* type, any* value) {
    return type_cast_func_nil(type, value);
}

static bool type_check_func_int(struct type* type, any value) {
    return any_is_int(value);
}

static bool type_cast_func_int(struct type* type, any* value) {
    if (type_check_func_int(type, *value))
        return true;
    else if (any_is_char(*value)) {
        *value = any_from_int(any_to_char(*value));
        return true;
    }
    else
        return false;
}

static bool type_check_func_char(struct type* type, any value) {
    return any_is_char(value);
}

static bool type_cast_func_char(struct type* type, any* value) {
    if (type_check_func_char(type, *value))
        return true;
    else if (any_is_int(*value)) {
        *value = any_from_char(any_to_char(*value));
        return true;
    }
    else
        return false;
}

static bool type_check_func_string(struct type* type, any value) {
    return any_is_obj(value, OBJ_TYPE_STRING) || any_is_nil(value);
}

static bool type_cast_func_string(struct type* type, any* value) {
    size_t  size;
    char    c[8];

    if (any_is_char(*value)) {
        size    = utf8_encode(any_to_char(*value), c);
        c[size] = '\0';
        *value  = any_from_ptr(string_new(typeset_raven(type_typeset(type)), c));
        return true;
    }
    return type_check_func_string(type, *value);
}

static bool type_check_func_symbol(struct type* type, any value) {
    return any_is_obj(value, OBJ_TYPE_SYMBOL) || any_is_nil(value);
}

static bool type_cast_func_symbol(struct type* type, any* value) {
    struct symbol*  symbol;

    if (any_is_obj(*value, OBJ_TYPE_STRING)) {
        symbol = raven_find_symbol(typeset_raven(type_typeset(type)),
                                   string_contents(any_to_ptr(*value)));
        *value = any_from_ptr(symbol);
        return true;
    }
    return type_check_func_symbol(type, *value);
}

static bool type_check_func_object(struct type* type, any value) {
    return any_is_obj(value, OBJ_TYPE_OBJECT) || any_is_nil(value);
}

static bool type_cast_func_object(struct type* type, any* value) {
    return type_check_func_object(type, *value);
}

static bool type_check_func_funcref(struct type* type, any value) {
    return any_is_obj(value, OBJ_TYPE_FUNCREF) || any_is_nil(value);
}

static bool type_cast_func_funcref(struct type* type, any* value) {
    return type_check_func_funcref(type, *value);
}

static bool type_check_func_mapping(struct type* type, any value) {
    return any_is_obj(value, OBJ_TYPE_MAPPING) || any_is_nil(value);
}

static bool type_cast_func_mapping(struct type* type, any* value) {
    return type_check_func_mapping(type, *value);
}

void type_create(struct type*    type,
                 struct typeset* ts,
                 struct type*    parent,
                 type_check_func check,
                 type_cast_func  cast) {
    type->typeset    = ts;
    type->parent     = parent;
    type->check_func = check;
    type->cast_func  = cast;
}

void type_destroy(struct type* type) {
}

bool type_is_any(struct type* type) {
    if (type == NULL) return false;
    return typeset_type_any(type_typeset(type)) == type;
}

bool type_match(struct type* type, struct type* test) {
    if (type_is_any(type)) return true;
    else if (type_is_any(test)) return true; /* TODO: Add a strict mode */

    while (test != NULL) {
        if (type == test) return true;
        test = type_parent(test);
    }
    return false;
}

bool type_check(struct type* type, any value) {
    return type_type_check_func(type)(type, value);
}

bool type_cast(struct type* type, any* value) {
    return type_type_cast_func(type)(type, value);
}


void typeset_create(struct typeset* ts, struct raven* raven) {
    ts->raven = raven;
    type_create(&ts->void_type, ts, NULL, type_check_func_void, type_cast_func_void);
    type_create(&ts->any_type, ts, NULL, type_check_func_any, type_cast_func_any);
    type_create(&ts->int_type, ts, &ts->any_type, type_check_func_int, type_cast_func_int);
    type_create(&ts->char_type, ts, &ts->any_type, type_check_func_char, type_cast_func_char);
    type_create(&ts->string_type, ts, &ts->any_type, type_check_func_string, type_cast_func_string);
    type_create(&ts->symbol_type, ts, &ts->any_type, type_check_func_symbol, type_cast_func_symbol);
    type_create(&ts->object_type, ts, &ts->any_type, type_check_func_object, type_cast_func_object);
    type_create(&ts->funcref_type, ts, &ts->any_type, type_check_func_funcref, type_cast_func_funcref);
    type_create(&ts->mapping_type, ts, &ts->any_type, type_check_func_mapping, type_cast_func_mapping);
}

void typeset_destroy(struct typeset* ts) {
    type_destroy(&ts->mapping_type);
    type_destroy(&ts->funcref_type);
    type_destroy(&ts->object_type);
    type_destroy(&ts->symbol_type);
    type_destroy(&ts->string_type);
    type_destroy(&ts->char_type);
    type_destroy(&ts->int_type);
    type_destroy(&ts->any_type);
    type_destroy(&ts->void_type);
}
