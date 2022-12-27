/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"
#include "../../raven/raven.h"

#include "../../platform/abstraction/socket.h"

#include "../../runtime/core/objects/connection.h"
#include "../../runtime/core/objects/funcref.h"
#include "../../runtime/vm/scheduler.h"
#include "../../runtime/vm/fiber.h"
#include "../../runtime/vm/interpreter.h"

#include "server.h"


bool server_open_socket(int port, int* socket_loc) {
    int                 fd;
    struct sockaddr_in  addr_in;

    fd  = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        return false;
    }

    if (!pal_socket_reuseaddr(fd)) {
        pal_socket_close(fd);
        return false;
    }

    addr_in.sin_family      = AF_INET;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_port        = htons(port);

    if (bind(fd, (struct sockaddr*) &addr_in, sizeof(addr_in)) < 0) {
        pal_socket_close(fd);
        return false;
    }

    listen(fd, 3);

    *socket_loc = fd;
    return true;
}


void server_create(struct server* server, struct raven* raven) {
    server->raven               = raven;
    server->connections         = NULL;
    server->server_socket_count = 0;
}

void server_destroy(struct server* server) {
    while (server->connections != NULL)
        connection_detach_from_server(server->connections);
}

bool server_serve_on(struct server* server, int port) {
    if (server->server_socket_count >= RAVEN_SERVER_SOCKETS_MAX) return false;
    return server_open_socket(port, &server->server_sockets[server->server_socket_count++]);
}

static void server_send_socket_error_and_close(struct server* server, int fd) {
    const char*  msg = "\n    The MUD cannot take any connections right now.\n\n";

    pal_socket_write(fd, msg, strlen(msg), NULL);
    pal_socket_close(fd);
}

void server_accept(struct server* server, int socket_fd) {
    int                 fd;
    struct connection*  connection;
    struct raven*       raven;
    struct fiber*       fiber;
    struct funcref*     func;
    any                 connection_any;

    if (pal_socket_accept(socket_fd, &fd)) {
        raven = server_raven(server);
        func  = raven_vars(raven)->connect_func;
        if (func == NULL) {
            server_send_socket_error_and_close(server, fd);
        } else {
            connection = connection_new(server_raven(server), server, fd);
            if (connection == NULL) {
                /* TODO: Warning */
                close(fd);
            } else {
                fiber = scheduler_new_fiber(raven_scheduler(raven));
                if (fiber != NULL) {
                    fiber_set_connection(fiber, connection);
                    connection_set_fiber(connection, fiber);
                    connection_any = any_from_ptr(connection);
                    funcref_enter(func, fiber, &connection_any, 1);
                } // TODO: Else error!
            }
        }
    }
}

void server_tick(struct server* server, raven_timeval_t tv) {
    int                 maxfd;
    int                 retval;
    size_t              bytes;
    struct connection*  connection;
    unsigned int        index;
    fd_set              readable;
    char                buffer[1024];

    FD_ZERO(&readable);

    maxfd = 0;

    for (index = 0; index < server->server_socket_count; ++index) {
        if (server->server_sockets[index] > maxfd)
            maxfd = server->server_sockets[index];
        FD_SET(server->server_sockets[index], &readable);
    }

    for (connection = server->connections;
         connection != NULL;
         connection = connection_next(connection)) {
        FD_SET(connection_socket(connection), &readable);
        if (connection_socket(connection) > maxfd)
            maxfd = connection_socket(connection);
    }

    retval = select(maxfd + 1, &readable, NULL, NULL, &tv);

    if (retval > 0) {
        for (index = 0; index < server->server_socket_count; ++index) {
            if (FD_ISSET(server->server_sockets[index], &readable))
                server_accept(server, server->server_sockets[index]);
        }
        for (connection = server->connections;
             connection != NULL;
             connection = connection_next(connection)) {
            if (FD_ISSET(connection_socket(connection), &readable)) {
                if (pal_socket_read(connection_socket(connection), buffer, sizeof(buffer), &bytes)) {
                    connection_push_input(connection, buffer, (unsigned int) bytes);
                } else {
                    connection_endofinput(connection);
                }
            }
        }
    }
}
