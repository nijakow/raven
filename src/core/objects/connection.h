/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_SERVER_CONNECTION_H
#define RAVEN_SERVER_CONNECTION_H

#include "../../defs.h"
#include "../any.h"
#include "../../util/ringbuffer.h"

#include "../base_obj.h"


struct connection {
  struct base_obj     _;
  struct raven*       raven;
  struct server*      server;
  struct connection*  next;
  struct connection** prev;
  struct fiber*       fiber;
  int                 socket;
  struct ringbuffer   in_buffer;
  any                 player_object;
};

struct connection* connection_new(struct raven* raven,
                                  struct server* server,
                                  int socket);
void connection_mark(struct gc* gc, struct connection* connection);
void connection_del(struct connection* connection);

void connection_detach_from_server(struct connection* connection);

void connection_close_impl(struct connection* connection);
void connection_close(struct connection* connection);
void connection_endofinput(struct connection* connection);
void connection_input(struct connection* connection, char* b, unsigned int n);
void connection_output_str(struct connection* connection, const char* str);

static inline struct raven* connection_raven(struct connection* connection) {
  return connection->raven;
}

static inline struct server* connection_server(struct connection* connection) {
  return connection->server;
}

static inline struct connection*
    connection_next(struct connection* connection) {
  return connection->next;
}

static inline int connection_socket(struct connection* connection) {
  return connection->socket;
}

static inline struct fiber* connection_fiber(struct connection* connection) {
  return connection->fiber;
}

static inline void connection_set_fiber(struct connection* connection,
                                        struct fiber*      fiber) {
  connection->fiber = fiber;
}

static inline any connection_player_object(struct connection* connection) {
  return connection->player_object;
}

static inline void connection_set_player_object(struct connection* connection,
                                                any                value) {
  connection->player_object = value;
}

#endif
