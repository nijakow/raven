/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_SERVER_SERVER_H
#define RAVEN_SERVER_SERVER_H

#include "../defs.h"

#define RAVEN_SERVER_SOCKETS_MAX 8

struct server {
    struct raven*      raven;
    struct connection* connections;
    unsigned int       server_socket_count;
    int                server_sockets[RAVEN_SERVER_SOCKETS_MAX];
};

void server_create(struct server* server, struct raven* raven);
void server_destroy(struct server* server);

bool server_serve_on(struct server* server, int port);
void server_accept(struct server* server, int socket_fd);
void server_tick(struct server* server, raven_timeval_t tv);

static inline struct raven* server_raven(struct server* server) {
    return server->raven;
}

#endif
