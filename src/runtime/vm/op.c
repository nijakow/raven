/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../raven/raven.h"
#include "../../util/utf8.h"
#include "../../util/stringbuilder.h"

#include "../core/objects/array.h"
#include "../core/objects/mapping.h"
#include "../core/objects/object/object.h"
#include "../core/objects/string.h"
#include "../core/type.h"

#include "fiber.h"
#include "interpreter.h"

#include "op.h"

bool fiber_op_eq(struct fiber* fiber, any a, any b) {
    return any_eq(a, b);
}

bool fiber_op_ineq(struct fiber* fiber, any a, any b) {
    return !fiber_op_eq(fiber, a, b);
}

bool fiber_op_less(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_to_int(a) < any_to_int(b);
    else if (any_is_char(a) && any_is_char(b))
        return any_to_char(a) < any_to_char(b);
    else if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_obj(a, OBJ_TYPE_STRING))
        return string_less(any_to_ptr(a), any_to_ptr(b));
    else
        return false;
}

bool fiber_op_leq(struct fiber* fiber, any a, any b) {
    return fiber_op_less(fiber, a, b) || fiber_op_eq(fiber, a, b);
}

bool fiber_op_greater(struct fiber* fiber, any a, any b) {
    return !fiber_op_leq(fiber, a, b);
}

bool fiber_op_geq(struct fiber* fiber, any a, any b) {
    return !fiber_op_less(fiber, a, b);
}


static any fiber_op_add_str_char(struct fiber* fiber, struct string* str, raven_rune_t rune) {
    struct stringbuilder  sb;
    struct string*        result;

    stringbuilder_create(&sb);
    stringbuilder_append_str(&sb, string_contents(str));
    stringbuilder_append_rune(&sb, rune);
    result = string_new_from_stringbuilder(fiber_raven(fiber), &sb);
    stringbuilder_destroy(&sb);
    return any_from_ptr(result);
}

static any fiber_op_add_char_str(struct fiber* fiber, raven_rune_t rune, struct string* str) {
    struct stringbuilder  sb;
    struct string*        result;

    stringbuilder_create(&sb);
    stringbuilder_append_rune(&sb, rune);
    stringbuilder_append_str(&sb, string_contents(str));
    result = string_new_from_stringbuilder(fiber_raven(fiber), &sb);
    stringbuilder_destroy(&sb);
    return any_from_ptr(result);
}

static any fiber_op_add_char_char(struct fiber* fiber, raven_rune_t r1, raven_rune_t r2) {
    struct stringbuilder  sb;
    struct string*        result;

    stringbuilder_create(&sb);
    stringbuilder_append_rune(&sb, r1);
    stringbuilder_append_rune(&sb, r2);
    result = string_new_from_stringbuilder(fiber_raven(fiber), &sb);
    stringbuilder_destroy(&sb);
    return any_from_ptr(result);
}

any fiber_op_add(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_from_int(any_to_int(a) + any_to_int(b));
    else if (any_is_int(a) && any_is_char(b))
        return any_from_char(any_to_int(a) + any_to_char(b));
    else if (any_is_char(a) && any_is_int(b))
        return any_from_char(any_to_char(a) + any_to_int(b));
    else if (any_is_char(a) && any_is_char(b))
        return fiber_op_add_char_char(fiber, any_to_char(a), any_to_char(b));
    else if (any_is_char(a) && any_is_obj(b, OBJ_TYPE_STRING))
        return fiber_op_add_char_str(fiber, any_to_char(a), any_to_ptr(b));
    else if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_char(b))
        return fiber_op_add_str_char(fiber, any_to_ptr(a), any_to_char(b));
    else if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_obj(b, OBJ_TYPE_STRING))
        return any_from_ptr(string_append(fiber_raven(fiber),
                                          any_to_ptr(a),
                                          any_to_ptr(b)));
    else if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_nil(b))
        return a;
    else if (any_is_nil(a) && any_is_obj(b, OBJ_TYPE_STRING))
        return b;
    else if (any_is_obj(a, OBJ_TYPE_ARRAY) && any_is_obj(b, OBJ_TYPE_ARRAY))
        return any_from_ptr(array_join(fiber_raven(fiber),
                                       any_to_ptr(a),
                                       any_to_ptr(b)));
    else
        return any_nil();
}

any fiber_op_sub(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_from_int(any_to_int(a) - any_to_int(b));
    else if (any_is_char(a) && any_is_int(b))
        return any_from_char(any_to_char(a) - any_to_int(b));
    else if (any_is_char(a) && any_is_char(b))
        return any_from_int(any_to_char(a) - any_to_char(b));
    else
        return any_nil();
}

any fiber_op_mul(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_from_int(any_to_int(a) * any_to_int(b));
    else if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_int(b))
        return any_from_ptr(string_multiply(fiber_raven(fiber),
                                            any_to_ptr(a),
                                            any_to_int(b)));
    else if (any_is_int(a) && any_is_obj(b, OBJ_TYPE_STRING))
        return any_from_ptr(string_multiply(fiber_raven(fiber),
                                            any_to_ptr(b),
                                            any_to_int(a)));
    else
        return any_nil();
}

any fiber_op_div(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b)) {
        if (any_to_int(b) == 0) {
            fiber_crash_msg(fiber, "Division by zero!");
            return any_nil();
        }
        return any_from_int(any_to_int(a) / any_to_int(b));
    } else {
        return any_nil();
    }
}

any fiber_op_mod(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b)) {
        if (any_to_int(b) == 0) {
            fiber_crash_msg(fiber, "Division by zero!");
            return any_nil();
        }
        return any_from_int(any_to_int(a) % any_to_int(b));
    } else {
        return any_nil();
    }
}

any fiber_op_negate(struct fiber* fiber, any a) {
    if (any_is_int(a))
        return any_from_int(-any_to_int(a));
    else
        return any_nil();
}

any fiber_op_bitand(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_from_int(any_to_int(a) & any_to_int(b));
    else
        return any_nil();
}

any fiber_op_bitor(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_from_int(any_to_int(a) | any_to_int(b));
    else
        return any_nil();
}

any fiber_op_leftshift(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_from_int(any_to_int(a) << any_to_int(b));
    else {
        fiber_push(fiber, a);
        fiber_push(fiber, b);
        fiber_send(fiber, raven_find_symbol(fiber_raven(fiber), "operator<<"), 1);
        return any_nil();   // TODO: The value shouldn't be returned, but taken from the accu
    }
}

any fiber_op_rightshift(struct fiber* fiber, any a, any b) {
    if (any_is_int(a) && any_is_int(b))
        return any_from_int(any_to_int(a) >> any_to_int(b));
    else {
        fiber_push(fiber, a);
        fiber_push(fiber, b);
        fiber_send(fiber, raven_find_symbol(fiber_raven(fiber), "operator>>"), 1);
        return any_nil();   // TODO: The value shouldn't be returned, but taken from the accu
    }
}

any fiber_op_index(struct fiber* fiber, any a, any b) {
    any  result;

    if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_int(b)) {
        return any_from_char(string_at_rune(any_to_ptr(a), any_to_int(b)));
    } else if (any_is_obj(a, OBJ_TYPE_ARRAY) && any_is_int(b)) {
        return array_get(any_to_ptr(a), any_to_int(b));
    } else if (any_is_obj(a, OBJ_TYPE_MAPPING)) {
        mapping_get(any_to_ptr(a), b, &result);
        return result;
    } else if (any_is_int(a) && any_is_int(b)) {
        return any_from_int((any_to_int(a) & (1 << any_to_int(b))) != 0);
    } else {
        return any_nil();
    }
}

any fiber_op_index_assign(struct fiber* fiber, any a, any b, any c) {
    if (any_is_obj(a, OBJ_TYPE_ARRAY) && any_is_int(b)) {
        array_put(any_to_ptr(a), any_to_int(b), c);
        return c;
    } else if (any_is_obj(a, OBJ_TYPE_MAPPING)) {
        mapping_put(any_to_ptr(a), b, c);
        return c;
    } else if (any_is_int(a) && any_is_int(b)) {
        return any_from_int(any_to_int(a) | (any_to_int(c) << any_to_int(b)));
    } else {
        return any_nil();
    }
}

any fiber_op_range(struct fiber* fiber, any a, any b, any c) {
    // Extract a range [b..c] from a, an array or string
    // b and c must be integers

    int  start;
    int  end;

    if (any_is_int(b) && any_is_int(c)) {
        start = any_to_int(b);
        end   = any_to_int(c);

        if (any_is_obj(a, OBJ_TYPE_STRING)) {
            return any_from_ptr(string_substr(any_to_ptr(a), start, end, fiber_raven(fiber)));
        } // TODO: Add support for arrays
    }
    return any_nil();
}

any fiber_op_deref(struct fiber* fiber, any a) {
    return any_nil();
}

any fiber_op_sizeof(struct fiber* fiber, any a) {
    return any_from_int(any_op_sizeof(a));
}

any fiber_op_new(struct fiber* fiber, any a) {
    struct blueprint* blueprint;
    struct string*    string;
    struct object*    object;
    struct object*    result;

    if (any_is_obj(a, OBJ_TYPE_STRING)) {
        string    = any_to_ptr(a);
        blueprint = raven_get_blueprint(fiber_raven(fiber),
                                        string_contents(string),
                                        true);
    } else if (any_is_obj(a, OBJ_TYPE_OBJECT)) {
        object    = any_to_ptr(a);
        blueprint = object_blueprint(object);
    } else if (any_is_obj(a, OBJ_TYPE_BLUEPRINT)) {
        blueprint = any_to_ptr(a);
    } else {
        return any_nil();
    }

    if (blueprint != NULL) {
        result = object_new(fiber_raven(fiber), blueprint);
        if (result != NULL)
            return any_from_ptr(result);
    }
    return any_nil();
}

bool fiber_op_typecheck(struct fiber* fiber, any a, struct type* type) {
    return type_check(type, a);
}

any fiber_op_typecast(struct fiber* fiber, any a, struct type* type) {
    if (!type_cast(type, &a))
        fiber_crash_msg(fiber, "Typecast failed!");
    return a;
}
