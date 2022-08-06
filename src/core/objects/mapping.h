#ifndef RAVEN_CORE_OBJECTS_MAPPING_H
#define RAVEN_CORE_OBJECTS_MAPPING_H

#include "../any.h"
#include "base_obj.h"

struct mapping_entry {
  struct mapping_entry*  next;
  any                    key;
  any                    value;
};

struct mapping {
  struct base_obj        _;
  struct mapping_entry*  entries;
};

struct mapping* mapping_new(struct raven* raven);
void            mapping_mark(struct gc* gc, struct mapping* mapping);
void            mapping_del(struct mapping* mapping);

unsigned int    mapping_size(struct mapping* mapping);

bool mapping_get(struct mapping* mapping, any key, any* value);
bool mapping_put(struct mapping* mapping, any key, any value);

struct array* mapping_keys(struct mapping* mapping, struct raven* raven);

#endif
