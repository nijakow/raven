/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"
#include "../../raven.h"

#include "../../util/memory.h"

#include "array.h"

#include "mapping.h"


struct obj_info MAPPING_INFO = {
  .type  = OBJ_TYPE_MAPPING,
  .mark  = (mark_func) mapping_mark,
  .del   = (del_func)  mapping_del,
  .stats = (stats_func) NULL
};

struct mapping* mapping_new(struct raven* raven) {
  struct mapping* mapping;

  mapping = base_obj_new(raven_objects(raven),
                         &MAPPING_INFO,
                         sizeof(struct mapping));

  if (mapping != NULL) {
    mapping->entry_count = 0;
    mapping->entries     = NULL;
  }

  return mapping;
}

void mapping_mark(struct gc* gc, struct mapping* mapping) {
  unsigned int  index;

  for (index = 0; index < mapping->entry_count; index++) {
    gc_mark_any(gc, mapping->entries[index].key);
    gc_mark_any(gc, mapping->entries[index].value);
  }

  base_obj_mark(gc, &mapping->_);
}

void mapping_del(struct mapping* mapping) {
  if (mapping->entries != NULL)
    memory_free(mapping->entries);
  base_obj_del(&mapping->_);
}


static bool mapping_find_entry(struct mapping*        mapping,
                               any                    key,
                               struct mapping_entry** entry_loc) {
  struct mapping_entry*  entry;
  unsigned int           index;

  for (index = 0; index < mapping->entry_count; index++) {
    entry = &mapping->entries[index];
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

static bool mapping_find_empty_entry(struct mapping*        mapping,
                                     struct mapping_entry** entry_loc) {
  struct mapping_entry*  entry;
  unsigned int           index;

  for (index = 0; index < mapping->entry_count; index++) {
    entry = &mapping->entries[index];
    if (any_is_nil(entry->value)) {
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
  return mapping->entry_count;
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
  struct mapping_entry*  entries;
  unsigned int           new_size;

  if (mapping_find_entry(mapping, key, &entry))
    entry->value = value;
  else if (mapping_find_empty_entry(mapping, &entry)) {
    entry->key   = key;
    entry->value = value;
  } else {
    new_size = (mapping->entry_count == 0) ? 4 : (mapping->entry_count * 2);
    entries  = memory_realloc(mapping->entries,
                              new_size * sizeof(struct mapping_entry));
    if (entries == NULL)
      return false;
    else {
      mapping->entries = entries;
      mapping->entries[mapping->entry_count].key   = key;
      mapping->entries[mapping->entry_count].value = value;
      mapping->entry_count++;
      while (mapping->entry_count < new_size) {
        mapping->entries[mapping->entry_count].key   = any_nil();
        mapping->entries[mapping->entry_count].value = any_nil();
        mapping->entry_count++;
      }
    }
  }

  return true;
}

struct array* mapping_keys(struct mapping* mapping, struct raven* raven) {
  struct array* keys;
  unsigned int  index;

  keys = array_new(raven, 0);

  if (keys != NULL) {
    for (index = 0; index < mapping->entry_count; index++) {
      if (!(any_is_nil(mapping->entries[index].value)))
        array_append(keys, mapping->entries[index].key);
    }
  }

  return keys;
}
