#ifndef RAVEN_DEFS_H
#define RAVEN_DEFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/*
 * Every object in the system gets an obj_type, so that we can
 * distinguish what type of object a variable is pointing to.
 */
enum obj_type {
  OBJ_TYPE_OBJECT,
  OBJ_TYPE_SYMBOL,
  OBJ_TYPE_STRING,
  OBJ_TYPE_FUNCTION,
  OBJ_TYPE_ARRAY,
  OBJ_TYPE_MAPPING,
  OBJ_TYPE_FUNCREF,
  OBJ_TYPE_CONNECTION,
  OBJ_TYPE_BLUEPRINT
};

/*
 * t_bc and t_wc are used to represent bytecodes and
 * wordcodes, respectively. They are used in the bytecode
 * interpreter as instruction types.
 */
typedef uint8_t t_bc; /* byte code */
typedef int16_t t_wc; /* word code (must be signed!) */


/*
 *     I m p o r t a n t   S t r u c t s   a n d   F u n c t i o n s
 */

struct raven;

struct log;

struct type;
struct blueprint;

struct base_obj;
struct symbol;
struct string;
struct function;

struct scheduler;
struct fiber;
struct frame;

struct filesystem;
struct file;

struct server;

struct gc;

void gc_mark_ptr(struct gc* gc, void* ptr);

#endif
