/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../util/memory.h"

#include "array.h"

#include "mapping.h"


struct obj_info MAPPING_INFO = {
  .type = OBJ_TYPE_MAPPING,
  .mark = (mark_func) mapping_mark,
  .del  = (del_func)  mapping_del
};

struct mapping* mapping_new(struct raven* raven) {
  struct mapping* mapping;

  mapping = base_obj_new(raven, &MAPPING_INFO, sizeof(struct mapping));

  if (mapping != NULL) {
    mapping->entries = NULL;
  }

  return mapping;
}

void mapping_mark(struct gc* gc, struct mapping* mapping) {
  struct mapping_entry*  entry;

  for (entry = mapping->entries; entry != NULL; entry = entry->next) {
    gc_mark_any(gc, entry->key);
    gc_mark_any(gc, entry->value);
  }

  base_obj_mark(gc, &mapping->_);
}

void mapping_del(struct mapping* mapping) {
  struct mapping_entry*  entry;

  while (mapping->entries != NULL) {
    entry            = mapping->entries;
    mapping->entries = entry->next;
    memory_free(entry);
  }
  base_obj_del(&mapping->_);
}


static bool mapping_find_entry(struct mapping*        mapping,
                               any                    key,
                               struct mapping_entry** entry_loc) {
  struct mapping_entry*  entry;

  for (entry = mapping->entries; entry != NULL; entry = entry->next) {
    if (any_eq(entry->key, key)) {
      if (entry_loc != NULL) {
        *entry_loc = entry;
        return true;
      }
    }
  }

  if (entry_loc != NULL)
    *entry_loc = NULL;
  return false;
}

unsigned int mapping_size(struct mapping* mapping) {
  struct mapping_entry*  entry;
  unsigned int           count;

  count = 0;

  for (entry = mapping->entries; entry != NULL; entry = entry->next) {
    count++;
  }

  return count;
}

bool mapping_get(struct mapping* mapping, any key, any* value) {
  struct mapping_entry*  entry;

  if (mapping_find_entry(mapping, key, &entry)) {
    if (value != NULL)
      *value = entry->value;
    return true;
  } else {
    if (value != NULL)
      *value = any_nil();
    return false;
  }
}

bool mapping_put(struct mapping* mapping, any key, any value) {
  struct mapping_entry*  entry;

  if (mapping_find_entry(mapping, key, &entry))
    entry->value = value;
  else {
    entry = memory_alloc(sizeof(struct mapping_entry));
    if (entry == NULL) return false;
    entry->key       = key;
    entry->value     = value;
    entry->next      = mapping->entries;
    mapping->entries = entry;
  }

  return true;
}

struct array* mapping_keys(struct mapping* mapping, struct raven* raven) {
  struct array*          keys;
  struct mapping_entry*  entry;

  keys = array_new(raven, 0);

  if (keys != NULL) {
    for (entry = mapping->entries; entry != NULL; entry = entry->next) {
      array_append(keys, entry->key);
    }
  }

  return keys;
}
