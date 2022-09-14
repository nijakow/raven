#ifndef RAVEN_RAVEN_CHANNEL_H
#define RAVEN_RAVEN_CHANNEL_H

#include "../util/log.h"

struct channel {
  char*        name;
  struct log   log;
};

void channel_create(struct channel* channel, const char* name);
void channel_destroy(struct channel* channel);

void channel_printf(struct channel* channel,
                    const char* format,
                    ...);

#endif
