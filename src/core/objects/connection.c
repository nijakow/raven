/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../defs.h"

#include "../../server/server.h"
#include "../../vm/fiber.h"

#include "string.h"

#include "connection.h"


struct obj_info CONNECTION_INFO = {
  .type = OBJ_TYPE_CONNECTION,
  .mark = (mark_func) connection_mark,
  .del  = (del_func)  connection_del
};


static void connection_create(struct connection* connection,
                              struct server*     server,
                              int                socket) {
  connection->server  =  server;
  connection->socket  =  socket;
  ringbuffer_create(&connection->in_buffer);

  connection->next    =  server->connections;
  connection->prev    = &server->connections;
  if (server->connections != NULL)
    server->connections->prev = &connection->next;
  server->connections =  connection;
}

static void connection_destroy(struct connection* connection) {
  if (connection->socket >= 0)
    close(connection->socket);
  ringbuffer_destroy(&connection->in_buffer);
  if (connection->next != NULL)
    connection->next->prev = connection->prev;
  *(connection->prev) = connection->next;
}

struct connection* connection_new(struct raven*  raven,
                                  struct server* server,
                                  int            socket) {
  struct connection*  connection;

  connection = base_obj_new(raven, &CONNECTION_INFO, sizeof(struct connection));

  if (connection != NULL) {
    connection_create(connection, server, socket);
  }

  return connection;
}

void connection_mark(struct gc* gc, struct connection* connection) {
  base_obj_mark(gc, &connection->_);
}

void connection_del(struct connection* connection) {
  connection_destroy(connection);
  base_obj_del(&connection->_);
}


void connection_detach_from_server(struct connection* connection) {
  connection->server = NULL;
}

void connection_close(struct connection* connection) {
  /* TODO */
}

void connection_endofinput(struct connection* connection) {
  /* TODO */
}

void connection_input(struct connection* connection, char* b, unsigned int n) {
  char*          str;
  unsigned int   i;
  struct string* string;

  for (i = 0; i < n; i++) {
    ringbuffer_write(&connection->in_buffer, b[i]);
  }

  str = ringbuffer_line(&connection->in_buffer);

  if (str != NULL) {
    string = string_new(server_raven(connection_server(connection)), str);
    if (string != NULL) {
      if (connection_fiber(connection) != NULL) {
        fiber_push_input(connection_fiber(connection), string);
      }
    }
  }
}

void connection_output_str(struct connection* connection, const char* str) {
  size_t  len;

  len = strlen(str);
  write(connection_socket(connection), str, len);
}
