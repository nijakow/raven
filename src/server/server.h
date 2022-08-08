/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_SERVER_SERVER_H
#define RAVEN_SERVER_SERVER_H

#include "../defs.h"

struct server {
  struct raven*      raven;
  struct connection* connections;
  int                server_socket;
};

void server_create(struct server* server, struct raven* raven);
void server_destroy(struct server* server);

bool server_serve_on(struct server* server, int port);
void server_accept(struct server* server);
void server_tick(struct server* server);

static inline struct raven* server_raven(struct server* server) {
  return server->raven;
}

static inline int server_serversocket(struct server* server) {
  return server->server_socket;
}

#endif
