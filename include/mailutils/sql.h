/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_SQL_H
#define _MAILUTILS_SQL_H

/* Loadable Modules Suppert */
#define __s_cat2__(a,b) a ## b 
#define __s_cat3__(a,b,c) a ## b ## c
#define RDL_EXPORT(module,name) __s_cat3__(module,_LTX_,name)

typedef int (*rdl_init_t) (void);
typedef void (*rdl_done_t) (void);

#ifdef _HAVE_LIBLTDL //FIXME: Remove leading _ when SQL + ltdl works
# define MU_DECL_SQL_DISPATCH_T(mod) \
  mu_sql_dispatch_t RDL_EXPORT(mod,dispatch_tab)
#else
# define MU_DECL_SQL_DISPATCH_T(mod) \
  mu_sql_dispatch_t __s_cat2__(mod,_dispatch_tab)
#endif

enum mu_sql_connection_state {
  mu_sql_not_connected,
  mu_sql_connected,
  mu_sql_query_run,
  mu_sql_result_available
};

typedef struct mu_sql_connection *mu_sql_connection_t;

struct mu_sql_connection
{
  int  interface;
  char *server;
  int  port;
  char *login;
  char *password;
  char *dbname;
  void *data;
  enum mu_sql_connection_state state;
};

typedef struct mu_sql_dispatch mu_sql_dispatch_t;

struct mu_sql_dispatch
{
  char *name;
  int port;
  
  int (*init) (mu_sql_connection_t conn);
  int (*destroy) (mu_sql_connection_t conn);

  int (*connect) (mu_sql_connection_t conn);
  int (*disconnect) (mu_sql_connection_t conn);

  int (*query) (mu_sql_connection_t conn, char *query);
  int (*store_result) (mu_sql_connection_t conn);
  int (*release_result) (mu_sql_connection_t conn);
  
  int (*num_tuples) (mu_sql_connection_t conn, size_t *np);
  int (*num_columns) (mu_sql_connection_t conn, size_t *np);

  int (*get_column) (mu_sql_connection_t conn, size_t nrow, size_t ncol,
		     char **pdata);

  const char *(*errstr) (mu_sql_connection_t conn);
};

/* Public interfaces */
int mu_sql_interface_index (char *name);

int mu_sql_connection_init (mu_sql_connection_t *conn, int interface,
			    char *server, int  port, char *login,
			    char *password, char *dbname);
int mu_sql_connection_destroy (mu_sql_connection_t *conn);

int mu_sql_connect (mu_sql_connection_t conn);
int mu_sql_disconnect (mu_sql_connection_t conn);

int mu_sql_query (mu_sql_connection_t conn, char *query);

int mu_sql_store_result (mu_sql_connection_t conn);
int mu_sql_release_result (mu_sql_connection_t conn);

int mu_sql_num_tuples (mu_sql_connection_t conn, size_t *np);
int mu_sql_num_columns (mu_sql_connection_t conn, size_t *np);

int mu_sql_get_column (mu_sql_connection_t conn, size_t nrow, size_t ncol,
		       char **pdata);

const char *mu_sql_strerror (mu_sql_connection_t conn);

#endif
