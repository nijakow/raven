/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_ANY_H
#define RAVEN_CORE_ANY_H

/*
 * An ANY is a data structure that can hold either integers,
 * floating point numbers or references to objects. It is
 * implemented by a tagged union.
 *
 * Usage:
 * {
 *    any variable;
 *
 *    variable = any_from_int(42);
 *
 *    if (any_is_int(variable)) {
 *        printf("Integer: %d!\n", any_to_int(variable));
 *    } else if (any_is_ptr(variable)) {
 *        printf("Pointer: %p!\n", any_to_ptr(variable));
 *    } else {
 *        printf("Something different!\n");
 *    }
 * }
 */

#include "../defs.h"

/*
 * The different tags, each corresponding to a field
 * in the union type below.
 */
enum any_type {
    ANY_TYPE_NIL,
    ANY_TYPE_PTR,
    ANY_TYPE_INT,
    ANY_TYPE_CHAR
};

#ifdef RAVEN_USE_COMPRESSED_ANY

#define RAVEN_ANY_TYPE_SHL(t) (((uintptr_t) t) << 62)
#define RAVEN_ANY_TYPE_SHR(t) ((enum any_type) ((t >> 62) & 0x03))
#define RAVEN_ANY_PAYLOAD(t) (t & ~(((uintptr_t) 0x03) << 62))

typedef uintptr_t any;

/*
 * Extract the type of an `any`.
 */
static inline enum any_type any_type(any a) {
    return RAVEN_ANY_TYPE_SHR(a);
}

/*
 * Create an `any` that holds `nil`.
 */
static inline any any_nil() {
    return RAVEN_ANY_TYPE_SHL(ANY_TYPE_NIL);
}

/*
 * Create an `any` that holds a pointer.
 */
static inline any any_from_ptr(void* ptr) {
    return RAVEN_ANY_TYPE_SHL(ANY_TYPE_PTR) | ((uintptr_t) ptr);
}

/*
 * Create an `any` that holds an integer.
 */
static inline any any_from_int(int integer) {
    return RAVEN_ANY_TYPE_SHL(ANY_TYPE_INT) | (integer & 0xffffffff);
}

/*
 * Create an `any` that holds a character.
 */
static inline any any_from_char(char ch) {
    return RAVEN_ANY_TYPE_SHL(ANY_TYPE_CHAR) | (ch & 0xffffffff);
}

/*
 * Convert an `any` into a pointer. No type checks.
 */
static inline void* any_to_ptr(any a) {
    return (void*) RAVEN_ANY_PAYLOAD(a);
}

/*
 * Convert an `any` into an integer.
 */
static inline int any_to_int(any a) {
    return (int) (RAVEN_ANY_PAYLOAD(a) & 0xffffffff);
}

/*
 * Convert an `any` into a character.
 */
static inline char any_to_char(any a) {
    return (char) (RAVEN_ANY_PAYLOAD(a) & 0xffffffff);
}

#else

/*
 * The union type.
 */
union any_value {
    void* pointer;
    int   integer;
    char  character;
};

/*
 * The tagged union.
 */
typedef struct {
    enum  any_type  type;
    union any_value value;
} any;

/*
 * Extract the type of an `any`.
 */
static inline enum any_type any_type(any a) {
    return a.type;
}

/*
 * Create an `any` that holds `nil`.
 */
static inline any any_nil() {
    any a;

    a.type = ANY_TYPE_NIL;
    return a;
}

/*
 * Create an `any` that holds a pointer.
 */
static inline any any_from_ptr(void* ptr) {
    any a;

    a.type          = ANY_TYPE_PTR;
    a.value.pointer = ptr;

    return a;
}

/*
 * Create an `any` that holds an integer.
 */
static inline any any_from_int(int integer) {
    any a;

    a.type          = ANY_TYPE_INT;
    a.value.integer = integer;

    return a;
}

/*
 * Create an `any` that holds a character.
 */
static inline any any_from_char(char ch) {
    any a;

    a.type            = ANY_TYPE_CHAR;
    a.value.character = ch;

    return a;
}

/*
 * Convert an `any` into a pointer. No type checks.
 */
static inline void* any_to_ptr(any a) {
    return a.value.pointer;
}

/*
 * Convert an `any` into an integer.
 */
static inline int any_to_int(any a) {
    return a.value.integer;
}

/*
 * Convert an `any` into a character.
 */
static inline char any_to_char(any a) {
    return a.value.character;
}

#endif

/*
 * Check if an `any` holds a specific type.
 */
static inline bool any_is(any a, enum any_type type) {
    return any_type(a) == type;
}

/*
 * Check if an `any` holds `nil`.
 */
static inline bool any_is_nil(any a) {
    return any_is(a, ANY_TYPE_NIL);
}

/*
 * Check if an `any` holds a pointer.
 */
static inline bool any_is_ptr(any a) {
    return any_is(a, ANY_TYPE_PTR);
}

/*
 * Check if an `any` holds an integer.
 */
static inline bool any_is_int(any a) {
    return any_is(a, ANY_TYPE_INT);
}

/*
 * Check if an `any` holds a character.
 */
static inline bool any_is_char(any a) {
    return any_is(a, ANY_TYPE_CHAR);
}

/*
 * Check if an `any` holds a value that can be considered `true`.
 */
static inline bool any_bool_check(any a) {
    if (any_is_nil(a))
        return false;
    else if (any_is_int(a) && !any_to_int(a))
        return false;
    else if (any_is_char(a) && !any_to_char(a))
        return false;
    return true;
}

/*
 * Check if an `any` equals another.
 */
bool any_eq(any a, any b);

/*
 * Check if an `any` holds a pointer to a specific object type.
 */
bool any_is_obj(any a, enum obj_type type);

/*
 * The `sizeof` operation on an `any`.
 */
unsigned int any_op_sizeof(any a);

/*
 * Extracts the blueprint of the object held by an `any`.
 */
struct blueprint* any_get_blueprint(any a);

/*
 * Extracts a member function of the object held by an `any`.
 */
struct function* any_resolve_func(any a, struct symbol* message, bool allow_private);

#endif
