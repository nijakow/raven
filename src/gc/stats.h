#ifndef RAVEN_GC_STATS_H
#define RAVEN_GC_STATS_H

#include "../defs.h"

struct obj_stats {
  unsigned long  count;
  size_t         size;
};

void obj_stats_create(struct obj_stats* stats);
void obj_stats_destroy(struct obj_stats* stats);

void obj_stats_inc_count(struct obj_stats* stats);


struct stats {
  struct obj_stats object_stats[OBJ_TYPE_MAX];
};

void stats_create(struct stats* stats);
void stats_destroy(struct stats* stats);

void stats_gaze_at(struct stats* stats, void* object);

#endif
