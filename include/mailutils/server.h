/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _MAILUTILS_SERVER_H
#define _MAILUTILS_SERVER_H

#include <mailutils/types.h>

typedef int (*mu_conn_loop_fp) (int fd, void *conn_data, void *server_data);
typedef void (*mu_conn_free_fp) (void *conn_data, void *server_data);
typedef int (*mu_server_idle_fp) (void *server_data);
typedef void (*mu_server_free_fp) (void *server_data);

#define MU_SERVER_SUCCESS    0
#define MU_SERVER_CLOSE_CONN 1
#define MU_SERVER_SHUTDOWN   2

int mu_server_run (mu_server_t srv);
int mu_server_create (mu_server_t *psrv);
int mu_server_destroy (mu_server_t *psrv);
int mu_server_set_idle (mu_server_t srv, mu_server_idle_fp fp);
int mu_server_set_data (mu_server_t srv, void *data, mu_server_free_fp fp);
int mu_server_add_connection (mu_server_t srv,
			      int fd, void *data,
			      mu_conn_loop_fp loop, mu_conn_free_fp free);
struct timeval;
int mu_server_set_timeout (mu_server_t srv, struct timeval *to);
int mu_server_count (mu_server_t srv, size_t *pcount);


/* TCP server */
struct sockaddr_in;
typedef int (*mu_tcp_server_conn_fp) (int fd, struct sockaddr_in *s,
				      void *server_data, void *call_data,
				      mu_tcp_server_t srv);
typedef int (*mu_tcp_server_intr_fp) (void *data, void *call_data);
typedef void (*mu_tcp_server_free_fp) (void *data);


int mu_tcp_server_create (mu_tcp_server_t *psrv, struct sockaddr_in *addr);
int mu_tcp_server_destroy (mu_tcp_server_t *psrv);
int mu_tcp_server_set_debug (mu_tcp_server_t srv, mu_debug_t debug);
int mu_tcp_server_get_debug (mu_tcp_server_t srv, mu_debug_t *pdebug);
int mu_tcp_server_set_backlog (mu_tcp_server_t srv, int backlog);
int mu_tcp_server_set_ident (mu_tcp_server_t srv, const char *ident);
int mu_tcp_server_set_acl (mu_tcp_server_t srv, mu_acl_t acl);
int mu_tcp_server_set_conn (mu_tcp_server_t srv, mu_tcp_server_conn_fp conn);
int mu_tcp_server_set_intr (mu_tcp_server_t srv, mu_tcp_server_intr_fp intr);
int mu_tcp_server_set_data (mu_tcp_server_t srv,
			    void *data, mu_tcp_server_free_fp free);
int mu_tcp_server_open (mu_tcp_server_t srv);
int mu_tcp_server_shutdown (mu_tcp_server_t srv);
int mu_tcp_server_accept (mu_tcp_server_t srv, void *call_data);
int mu_tcp_server_loop (mu_tcp_server_t srv, void *call_data);
int mu_tcp_server_get_fd (mu_tcp_server_t srv);
int mu_tcp_server_get_sockaddr (mu_tcp_server_t srv, struct sockaddr_in *s);

#endif
