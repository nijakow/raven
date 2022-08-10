/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../core/objects/array.h"
#include "../core/objects/mapping.h"
#include "../core/objects/object.h"
#include "../core/objects/string.h"
#include "../core/type.h"
#include "../raven.h"

#include "fiber.h"

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
  else
    return false;
}

bool fiber_op_leq(struct fiber* fiber, any a, any b) {
  if (any_is_int(a) && any_is_int(b))
    return any_to_int(a) <= any_to_int(b);
  else if (any_is_char(a) && any_is_char(b))
    return any_to_char(a) <= any_to_char(b);
  else
    return false;
}

bool fiber_op_greater(struct fiber* fiber, any a, any b) {
  if (any_is_int(a) && any_is_int(b))
    return any_to_int(a) > any_to_int(b);
  else if (any_is_char(a) && any_is_char(b))
    return any_to_char(a) > any_to_char(b);
  else
    return false;
}

bool fiber_op_geq(struct fiber* fiber, any a, any b) {
  if (any_is_int(a) && any_is_int(b))
    return any_to_int(a) >= any_to_int(b);
  else if (any_is_char(a) && any_is_char(b))
    return any_to_char(a) >= any_to_char(b);
  else
    return false;
}


any fiber_op_add(struct fiber* fiber, any a, any b) {
  if (any_is_int(a) && any_is_int(b))
    return any_from_int(any_to_int(a) + any_to_int(b));
  else if (any_is_int(a) && any_is_char(b))
    return any_from_char(any_to_int(a) + any_to_char(b));
  else if (any_is_char(a) && any_is_int(b))
    return any_from_char(any_to_char(a) + any_to_int(b));
  else if (any_is_char(a) && any_is_char(b))
    return any_from_char(any_to_char(a) + any_to_char(b));
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
  else if (any_is_int(a) && any_is_char(b))
    return any_from_char(any_to_int(a) - any_to_char(b));
  else if (any_is_char(a) && any_is_int(b))
    return any_from_char(any_to_char(a) - any_to_int(b));
  else if (any_is_char(a) && any_is_char(b))
    return any_from_char(any_to_char(a) - any_to_char(b));
  else
    return any_nil();
}

any fiber_op_mul(struct fiber* fiber, any a, any b) {
  if (any_is_int(a) && any_is_int(b))
    return any_from_int(any_to_int(a) * any_to_int(b));
  else
    return any_nil();
}

any fiber_op_div(struct fiber* fiber, any a, any b) {
  if (any_is_int(a) && any_is_int(b))
    return any_from_int(any_to_int(a) / any_to_int(b));
  else
    return any_nil();
}

any fiber_op_mod(struct fiber* fiber, any a, any b) {
  if (any_is_int(a) && any_is_int(b))
    return any_from_int(any_to_int(a) % any_to_int(b));
  else
    return any_nil();
}

any fiber_op_negate(struct fiber* fiber, any a) {
  if (any_is_int(a))
    return any_from_int(-any_to_int(a));
  else
    return any_nil();
}

any fiber_op_index(struct fiber* fiber, any a, any b) {
  any  result;

  if (any_is_obj(a, OBJ_TYPE_STRING) && any_is_int(b)) {
    return any_from_char(string_at(any_to_ptr(a), any_to_int(b)));
  } else if (any_is_obj(a, OBJ_TYPE_ARRAY) && any_is_int(b)) {
    return array_get(any_to_ptr(a), any_to_int(b));
  } else if (any_is_obj(a, OBJ_TYPE_MAPPING)) {
    mapping_get(any_to_ptr(a), b, &result);
    return result;
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
  } else {
    return any_nil();
  }
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

  if (!any_is_obj(a, OBJ_TYPE_STRING))
    return any_nil();
  else {
    string    = any_to_ptr(a);
    blueprint = raven_get_blueprint(fiber_raven(fiber),
                                    string_contents(string));
    if (blueprint != NULL) {
      object = object_new(fiber_raven(fiber), blueprint);
      if (object != NULL)
        return any_from_ptr(object);
    }
  }
  return any_nil();
}

bool fiber_op_typecheck(struct fiber* fiber, any a, struct type* type) {
  return type_check(type, a);
}

any fiber_op_typecast(struct fiber* fiber, any a, struct type* type) {
  if (!type_cast(type, &a))
    fiber_crash(fiber);
  return a;
}
