/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"
#include "../../raven.h"

#include "../../util/stringbuilder.h"
#include "../../util/memory.h"

#include "string.h"

struct obj_info STRING_INFO = {
  .type = OBJ_TYPE_STRING,
  .mark = string_mark,
  .del  = string_del
};

char* strjoin(const char* s1, const char* s2) {
  size_t l1;
  size_t l2;
  char*  result;

  l1 = strlen(s1);
  l2 = strlen(s2);
  result = memory_alloc(l1 + l2 + 1);

  if (result != NULL) {
    memcpy(result, s1, l1);
    memcpy(result + l1, s2, l2 + 1);
  }

  return result;
}

struct string* string_new(struct raven* raven, const char* contents) {
  struct string*  string;

  string = base_obj_new(raven_objects(raven),
                        &STRING_INFO,
                        sizeof(struct string));

  if (string != NULL) {
    string->contents = strdup(contents);
    string->length   = strlen(string->contents);
  }

  return string;
}

void string_mark(struct gc* gc, void* string) {
  struct string* str;

  str = string;
  base_obj_mark(gc, &str->_);
}

void string_del(void* string) {
  struct string* str;

  str = string;
  memory_free(str->contents);
  base_obj_del(&str->_);
}


struct string* string_append(struct raven*  raven,
                             struct string* a,
                             struct string* b) {
  struct string*  string;

  string = base_obj_new(raven_objects(raven),
                        &STRING_INFO,
                        sizeof(struct string));

  if (string != NULL) {
    string->contents = strjoin(a->contents, b->contents);
    string->length   = strlen(string->contents);
  }

  return string;
}

bool string_eq(struct string* a, struct string* b) {
  return strcmp(string_contents(a), string_contents(b)) == 0;
}

struct string* string_substr(struct string* string,
                             unsigned int   from,
                             unsigned int   to,
                             struct raven*  raven) {
  const char*           str;
  char*                 str2;
  unsigned int          i;
  struct stringbuilder  sb;

  stringbuilder_create(&sb);
  str = string_contents(string);
  for (i = 0; str[i] != '\0'; i++) {
    if (i >= from && i < to)
      stringbuilder_append_char(&sb, str[i]);
    if (i >= to) break;
  }
  stringbuilder_get(&sb, &str2);
  stringbuilder_destroy(&sb);
  if (str2 == NULL)
    return NULL;
  else
    return string_new(raven, str2);
}
