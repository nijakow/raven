#include "wrap.h"

static bool is_wrap_space(char c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

static bool is_wrap_stop(char c) {
  return is_wrap_space(c) || c == '\0';
}

void string_wrap_into(const char*           text,
                      unsigned int          margin,
                      struct stringbuilder* into) {
  unsigned int  index;
  unsigned int  next_index;
  unsigned int  position;
  unsigned int  width;

  index    = 0;
  position = 0;

  while (text[index] != '\0') {
    if (text[index] == '\n') {
      stringbuilder_append_char(into, '\n');
      position = 0;
      index++;
    } else if (is_wrap_space(text[index])) {
      stringbuilder_append_char(into, ' ');
      position++;
      index++;
    } else {
      for (next_index = index; !is_wrap_stop(text[next_index]); next_index++);
      width = next_index - index;
      if (position + width >= margin) {
        stringbuilder_append_char(into, '\n');
        position = 0;
      }
      while (index < next_index) {
        stringbuilder_append_char(into, text[index]);
        index++;
        position++;
        if (position >= margin) {
          stringbuilder_append_char(into, '\n');
          position = 0;
        }
      }
    }
  }
}
