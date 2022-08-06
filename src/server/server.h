#ifndef RAVEN_SERVER_SERVER_H
#define RAVEN_SERVER_SERVER_H

#include "../defs.h"

struct server {
  struct raven*      raven;
  struct connection* connections;
  int                server_socket;
};

void server_create(struct server* server, struct raven* raven, int port);
void server_destroy(struct server* server);

void server_accept(struct server* server);

void server_tick(struct server* server);

static inline struct raven* server_raven(struct server* server) {
  return server->raven;
}

static inline int server_serversocket(struct server* server) {
  return server->server_socket;
}

#endif
