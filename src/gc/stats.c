#include "stats.h"

#include "../core/base_obj.h"


void obj_stats_create(struct obj_stats* stats) {
  stats->size = 0;
}

void obj_stats_destroy(struct obj_stats* stats) {
  
}


void stats_create(struct stats* stats) {
  unsigned int  index;

  for (index = 0; index < OBJ_TYPE_MAX; index++) {
    obj_stats_create(&stats->object_stats[index]);
  }
}

void stats_destroy(struct stats* stats) {
  
}

void stats_gaze_at(struct stats* stats, void* object) {
  base_obj_dispatch_stats(object, &stats->object_stats[base_obj_type(object)]);
}

