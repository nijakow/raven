/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_DEFS_H
#define RAVEN_DEFS_H

/*
 * All external include headers. No other headers are used
 * in the system.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/*
 * I don't like #define-switches, but this one helps a lot when
 * you need to get detailed information about what the system
 * is doing.
 *
 * Enabling this will cause the server to print all kinds of
 * details about what's happening deep inside of its data
 * structures.
 */
#ifndef RAVEN_DEBUG_MODE
#  define RAVEN_DEBUG_MODE 0
#endif

/*
 * Color switch
 */
#define RAVEN_USE_VT100
#include "util/vt100.h"

/*
 * Optimizations for the `any` datatype
 */
#define RAVEN_USE_COMPRESSED_ANY

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

struct base_obj;
struct object_table;

struct type;
struct blueprint;

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

struct object_stats;

struct stringbuilder;

#endif
