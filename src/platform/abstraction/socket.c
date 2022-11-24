/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "socket.h"

bool pal_socket_accept(pal_socket_t socket, pal_socket_t* client) {
    *client = accept(socket, NULL, NULL);
    return (*client >= 0);
}

bool pal_socket_close(pal_socket_t socket) {
    return (close(socket) == 0);
}

bool pal_socket_read(pal_socket_t socket, void* data, size_t amount, size_t* amount_read) {
    ssize_t  size;

    if (amount == 0)
        return true;
    size = read(socket, data, amount);
    if (amount_read != NULL)
        *amount_read = (size_t) size;
    return (size > 0);
}

bool pal_socket_write(pal_socket_t socket, const void* data, size_t amount, size_t* amount_written) {
    ssize_t  size;

    if (amount == 0)
        return true;
    size = write(socket, data, amount);
    if (amount_written != NULL)
        *amount_written = (size_t) size;
    return (size > 0);
}

bool pal_socket_reuseaddr(pal_socket_t socket) {
    int one;

    one = 1;
    return (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) >= 0);
}
