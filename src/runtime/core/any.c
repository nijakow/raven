/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "objects/object/object.h"
#include "objects/array.h"
#include "objects/string.h"

#include "base_obj.h"
#include "blueprint.h"

#include "any.h"

bool any_eq(any a, any b) {
    if (any_is(a, ANY_TYPE_INT) && any_is(b, ANY_TYPE_CHAR))
        return any_to_int(a) == any_to_char(b);
    else if (any_is(a, ANY_TYPE_CHAR) && any_is(b, ANY_TYPE_INT))
        return any_to_char(a) == any_to_int(b);
    else if (any_type(a) != any_type(b))
        return false;
    switch (any_type(a))
    {
    case ANY_TYPE_NIL:
        return true;
    case ANY_TYPE_INT:
        return any_to_int(a) == any_to_int(b);
    case ANY_TYPE_CHAR:
        return any_to_char(a) == any_to_char(b);
    case ANY_TYPE_PTR:
        if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_obj(b, OBJ_TYPE_STRING))
            return string_eq(any_to_ptr(a), any_to_ptr(b));
        return any_to_ptr(a) == any_to_ptr(b);
    default:
        return false;
    }
}

bool any_is_obj(any a, enum obj_type type) {
    return any_is_ptr(a) && base_obj_is(any_to_ptr(a), type);
}

unsigned int any_op_sizeof(any a) {
    if (any_is_obj(a, OBJ_TYPE_STRING))
        return string_rune_length(any_to_ptr(a));
    else if (any_is_obj(a, OBJ_TYPE_ARRAY))
        return array_size(any_to_ptr(a));
    else
        return 0;
}

struct blueprint* any_get_blueprint(any a) {
    switch (any_type(a)) {
    case ANY_TYPE_PTR:
        if (base_obj_is(any_to_ptr(a), OBJ_TYPE_OBJECT))
            return object_blueprint(any_to_ptr(a));
        break;
    default:
        break;
    }
    return NULL;
}

bool any_resolve_func_and_page(any a, struct object_page_and_function* result, struct symbol* message, unsigned int args, bool allow_private) {
    // TODO: Set result (if the pointer is not NULL)
    return false;
}
