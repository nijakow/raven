/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_FS_FILE_H
#define RAVEN_FS_FILE_H

#include "../defs.h"
#include "../util/stringbuilder.h"

struct file {
  struct filesystem*  fs;
  struct file*        next;
  struct file**       prev;

  struct file*        parent;
  struct file*        sibling;
  struct file*        children;

  struct blueprint*   blueprint;
  struct object*      object;

  char                name[];
};

struct file* file_new(struct filesystem* fs,
                      struct file*       parent,
                      const char*        name);
void file_delete(struct file* file);

void file_mark(struct gc* gc, struct file* file);

void file_load(struct file* file, const char* realpath);

struct file* file_resolve1(struct file* file, const char* name);
struct file* file_resolve(struct file* file, const char* name);

char* file_path(struct file* file);

bool file_cat(struct file* file, struct stringbuilder* into);
bool file_recompile(struct file* file, struct log* log);
struct blueprint* file_get_blueprint(struct file* file);
struct object* file_get_object(struct file* file);

static inline struct filesystem* file_fs(struct file* file) {
  return file->fs;
}

static inline struct file* file_parent(struct file* file) {
  return file->parent;
}

static inline struct file* file_sibling(struct file* file) {
  return file->sibling;
}

static inline struct file* file_children(struct file* file) {
  return file->children;
}

static inline char* file_name(struct file* file) {
  return file->name;
}

#endif
