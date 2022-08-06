#ifndef RAVEN_UTIL_STRINGBUILDER_H
#define RAVEN_UTIL_STRINGBUILDER_H

#include "../defs.h"

struct stringbuilder {
  unsigned int alloc;
  unsigned int fill;
  char*        data;
};

void stringbuilder_create(struct stringbuilder* sb);
void stringbuilder_destroy(struct stringbuilder* sb);

void stringbuilder_append_char(struct stringbuilder* sb, char c);
void stringbuilder_append_str(struct stringbuilder* sb, const char* str);

void stringbuilder_get(struct stringbuilder* sb, char** loc);

void stringbuilder_clear(struct stringbuilder* sb);

#endif
