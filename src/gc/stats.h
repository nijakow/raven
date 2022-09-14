#ifndef RAVEN_GC_STATS_H
#define RAVEN_GC_STATS_H

#include "../defs.h"

struct obj_stats {
  size_t size;
};

struct stats {
  struct obj_stats object_stats[OBJ_TYPE_MAX];
};

void stats_create(struct stats* stats);
void stats_destroy(struct stats* stats);

void stats_gaze_at(struct stats* stats, void* object);

#endif
