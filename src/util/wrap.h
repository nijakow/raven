#ifndef RAVEN_UTIL_WRAP_H
#define RAVEN_UTIL_WRAP_H

#include "stringbuilder.h"

void string_wrap_into(const char*           text,
                      unsigned int          margin,
                      struct stringbuilder* into);

#endif
