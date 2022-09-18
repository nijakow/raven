/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_UTIL_STRINGBUILDER_H
#define RAVEN_UTIL_STRINGBUILDER_H

#include "../defs.h"

/*
 * Stringbuilders are objects that help us with the
 * instantiation and concatenation of strings.
 *
 * Internally, they contain a resizable buffer. We
 * have access to functions which extract and append
 * strings from and to this stringbuilder.
 *
 * A default usecase would be appending three strings.
 * In "normal" C we would have to write something like
 * this:
 *
 *     strjoin(strjoin("a", "b"), "c");
 *
 * Due to the discarded return value of the first strjoin,
 * this implementation would leak memory, so the actual
 * version should rather look like this:
 *
 *     char* join3(char* a, char* b, char* c) {
 *         char* tmp;
 *         char* res;
 *
 *         tmp = strjoin(a, b);
 *         res = strjoin(tmp, c);
 *         free(tmp);
 *         return res;
 *     }
 *
 * This is all a bit clunky, especially since the same
 * problem would arise if we were now had to append
 * four strings:
 *
 *     strjoin(strjoin3("a", "b", "c"), "d"); // Ow!
 *
 * A stringbuilder can help here:
 *
 *     {
 *         struct stringbuilder  sb;
 *         char*                 res;
 *
 *         stringbuilder_create(&sb);
 *         stringbuilder_append_str(&sb, "a");
 *         stringbuilder_append_str(&sb, "b");
 *         stringbuilder_append_str(&sb, "c");
 *         stringbuilder_append_str(&sb, "d");
 *         res = stringbuilder_get(&sb);
 *         stringbuilder_destroy(&sb);
 *     }
 *
 * This frees us of the burden of keeping track of all
 * the memory allocations and deallocations that would
 * happen while building the resulting string. It also
 * allows us to join as many strings as we want,
 * without having to change the underlying pattern.
 *
 * Stringbuilders are used throughout raven wherever
 * there's any need for resizeable strings or where
 * strings are being constructed out of smaller strings.
 */

struct stringbuilder {
  unsigned int alloc;
  unsigned int fill;
  char*        data;
};

void stringbuilder_create(struct stringbuilder* sb);
void stringbuilder_destroy(struct stringbuilder* sb);

void stringbuilder_append_char(struct stringbuilder* sb, char c);
void stringbuilder_append_str(struct stringbuilder* sb, const char* str);

bool stringbuilder_get(struct stringbuilder* sb, char** loc);

void stringbuilder_clear(struct stringbuilder* sb);

static inline const char* stringbuilder_get_const(struct stringbuilder* sb) {
  if (sb->data == NULL) return "";
  return sb->data;
}

#endif
