/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../../defs.h"
#include "../../../raven/raven.h"
#include "../../../platform/abstraction/socket.h"
#include "../../../platform/server/server.h"
#include "../../../util/memory.h"

#include "../../core/objects/funcref.h"
#include "../../vm/fiber.h"

#include "string.h"

#include "connection.h"


struct obj_info CONNECTION_INFO = {
    .type  = OBJ_TYPE_CONNECTION,
    .mark  = (mark_func)  connection_mark,
    .del   = (del_func)   connection_del,
    .stats = (stats_func) base_obj_stats
};


static void connection_create(struct connection* connection,
                              struct server*     server,
                              int                socket) {
    connection->raven         = server_raven(server);
    connection->server        = server;
    connection->fiber         = NULL;
    connection->waiting_fiber = NULL;
    connection->socket        = socket;
    connection->player_object = any_nil();
    ringbuffer_create(&connection->in_buffer);

    connection->next    =  server->connections;
    connection->prev    = &server->connections;
    if (server->connections != NULL)
        server->connections->prev = &connection->next;
    server->connections =  connection;
}

static void connection_destroy(struct connection* connection) {
    connection_close_impl(connection);
    ringbuffer_destroy(&connection->in_buffer);
}

struct connection* connection_new(struct raven*  raven,
                                  struct server* server,
                                  int            socket) {
    struct connection*  connection;

    connection = base_obj_new(raven_objects(raven),
                              &CONNECTION_INFO,
                              sizeof(struct connection));

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
    if (connection->next != NULL) connection->next->prev = connection->prev;
    if (connection->prev != NULL)      *connection->prev = connection->next;
    connection->prev   = NULL;
    connection->next   = NULL;
    connection->server = NULL;
}

void connection_close_impl(struct connection* connection) {
    if (connection->socket >= 0) {
        close(connection->socket);
        connection->socket = -1;
    }
    connection_detach_from_server(connection);
}

void connection_close(struct connection* connection) {
    struct raven*    raven;
    struct funcref*  func;
    struct fiber*    fiber;
    any              connection_any;

    connection_close_impl(connection);

    raven          = connection_raven(connection);
    func           = raven_vars(raven)->disconnect_func;
    connection_any = any_from_ptr(connection);
    if (func != NULL) {
        fiber = fiber_new(raven_scheduler(raven));
        if (fiber != NULL) {
            funcref_enter(func, fiber, &connection_any, 1);
        } // TODO: Else error!
    }
}

void connection_endofinput(struct connection* connection) {
    connection_close(connection);
}

void connection_push_char(struct connection* connection, char c) {
    if (connection_waiting_fiber(connection) != NULL) {
        fiber_push_input(connection_waiting_fiber(connection), any_from_int((unsigned char) c));
        connection_set_waiting_fiber(connection, NULL);
    } else {
        ringbuffer_write(&connection->in_buffer, c);
    }
}

void connection_push_input(struct connection* connection, char* b, unsigned int n) {
    unsigned int  i;

    for (i = 0; i < n; i++) {
        connection_push_char(connection, b[i]);
    }
}

bool connection_pull_input(struct connection* connection, char* loc) {
    return ringbuffer_read(&connection->in_buffer, loc);
}

void connection_write_byte(struct connection* connection, char byte) {
    pal_socket_write(connection_socket(connection), &byte, 1, NULL);
}

void connection_write_cstr(struct connection* connection, const char* str) {
    while (*str != '\0') {
        connection_write_byte(connection, *str);
        str++;
    }
}
