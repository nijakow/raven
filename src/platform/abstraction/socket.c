/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "socket.h"

bool pal_socket_read(pal_socket_t socket, void* data, size_t amount) {
    if (amount == 0)
        return true;
    return (read(socket, data, amount) > 0);
}

bool pal_socket_write(pal_socket_t socket, void* data, size_t amount) {
    if (amount == 0)
        return true;
    return (write(socket, data, amount) > 0);
}
