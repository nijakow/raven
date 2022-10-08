/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../util/memory.h"

#include "channel.h"


void channel_create(struct channel* channel, const char* name) {
    channel->name = memory_strdup(name);
    log_create(&channel->log);
}

void channel_destroy(struct channel* channel) {
    memory_free(channel->name);
    log_destroy(&channel->log);
}

void channel_printf(struct channel* channel,
                    const char* format,
                    ...) {
    va_list  args;

    va_start(args, format);
    log_vprintf(&channel->log, format, args);
    va_end(args);
}

