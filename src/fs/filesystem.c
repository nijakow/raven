/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "file.h"

#include "filesystem.h"


void filesystem_create(struct filesystem* fs, struct raven* raven) {
  fs->raven  = raven;
  fs->files  = NULL;
  fs->root   = NULL;
  fs->root   = file_new(fs, NULL, "");
  fs->anchor = NULL;
}

void filesystem_destroy(struct filesystem* fs) {
  while (fs->files != NULL)
    file_delete(fs->files);
}

void filesystem_mark(struct gc* gc, struct filesystem* fs) {
  struct file*  file;

  for (file = fs->files; file != NULL; file = file->next) {
    file_mark(gc, file);
  }
}

struct file* filesystem_resolve(struct filesystem* fs, const char* path) {
  return file_resolve(fs->root, path);
}

struct blueprint* filesystem_get_blueprint(struct filesystem* fs,
                                           const char* path) {
  struct file*  file;

  file = filesystem_resolve(fs, path);
  if (file == NULL)
    return NULL;
  else {
    return file_get_blueprint(file);
  }
}

struct object* filesystem_basic_get_object(struct filesystem* fs, const char* path) {
  struct file*  file;

  file = filesystem_resolve(fs, path);
  if (file == NULL)
    return NULL;
  else {
    return file_get_object(file);
  }
}

struct object* filesystem_get_object_with_extension(struct filesystem* fs,
                                                    const char*        path,
                                                    const char*        extension) {
  unsigned int  index;
  char*         p;
  char          buffer[1024];

  p = buffer;

  for (index = 0; path[index]      != '\0'; index++) *(p++) = path[index];
  for (index = 0; extension[index] != '\0'; index++) *(p++) = extension[index];
  *p = '\0';

  return filesystem_basic_get_object(fs, buffer);
}

struct object* filesystem_get_object(struct filesystem* fs, const char* path) {
  struct object*  obj;

  obj = filesystem_get_object_with_extension(fs, path, ".lpc");
  if (obj != NULL) return obj;
  obj = filesystem_get_object_with_extension(fs, path, ".c");
  if (obj != NULL) return obj;
  return filesystem_basic_get_object(fs, path);
}

void filesystem_set_anchor(struct filesystem* fs, const char* anchor) {
  fs->anchor = anchor;
}

void filesystem_load(struct filesystem* fs) {
  file_load(filesystem_root(fs), filesystem_anchor(fs));
}
