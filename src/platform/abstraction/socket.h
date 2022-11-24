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

bool pal_socket_accept(pal_socket_t socket, pal_socket_t* client);
bool pal_socket_close(pal_socket_t socket);

bool pal_socket_read(pal_socket_t socket, void* data, size_t amount, size_t* amount_read);
bool pal_socket_write(pal_socket_t socket, const void* data, size_t amount, size_t* amount_written);

bool pal_socket_reuseaddr(pal_socket_t socket);

#endif
