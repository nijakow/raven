/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_PLATFORM_ABSTRACTION_SOCKET_H
#define RAVEN_PLATFORM_ABSTRACTION_SOCKET_H

#include "../../defs.h"

typedef int pal_socket_t;

bool pal_socket_read(pal_socket_t socket, void* data, size_t amount);
bool pal_socket_write(pal_socket_t socket, void* data, size_t amount);

#endif
