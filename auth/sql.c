/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/argp.h>
#include <mailutils/mu_auth.h>

#ifdef HAVE_MYSQL
# include "../MySql/MySql.h"
#endif

/* SQL */
/* FIXME: Underlying library needs rewriting */

int
mu_auth_sql_by_name (void *return_data, void *key,
		     void *unused_func_data, void *unused_call_data)
{
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }
#ifdef HAVE_MYSQL
  return mu_auth_system (return_data, getMpwnam (key));
#else
  errno = ENOSYS;
  return 1;
#endif
}

int
mu_auth_sql_by_uid (void *return_data, void *key,
		    void *unused_func_data, void *unused_call_data)
{
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

#ifdef HAVE_MYSQL
  return mu_auth_system (return_data, getMpwuid (key));
#else
  errno = ENOSYS;
  return 1;
#endif
}

#ifdef HAVE_MYSQL
# define ARG_SQL_GETPWNAM 1
# define ARG_SQL_GETPWUID 2
# define ARG_SQL_GETPASS 3
# define ARG_SQL_HOST 4
# define ARG_SQL_USER 5
# define ARG_SQL_PASSWD 6
# define ARG_SQL_DB 7
# define ARG_SQL_PORT 8

static struct argp_option mu_sql_argp_option[] = {
  {"sql-getpwnam", ARG_SQL_GETPWNAM, "QUERY", 0,
   "SQL query to retrieve a passwd entry based on username", 0},
  {"sql-getpwuid", ARG_SQL_GETPWUID, "QUERY", 0,
   "SQL query to retrieve a passwd entry based on UID", 0},
  {"sql-getpass", ARG_SQL_GETPASS, "QUERY", 0,
   "SQL query to retrieve a password from the database", 0},
  {"sql-host", ARG_SQL_HOST, "HOSTNAME", 0,
   "Name or IP of MySQL server to connect to", 0},
  {"sql-user", ARG_SQL_USER, "NAME", 0,
   "SQL user name", 0},
  {"sql-passwd", ARG_SQL_PASSWD, "STRING", 0,
   "SQL connection password", 0},
  {"sql-db", ARG_SQL_DB, "STRING", 0,
   "Name of the database to connect to", 0},
  {"sql-port", ARG_SQL_PORT, "NUMBER", 0,
   "Port to use", 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

char *sql_getpwnam_query;
char *sql_getpass_query;
char *sql_getpwuid_query;
char *sql_host = MHOST;
char *sql_user = MUSER;
char *sql_passwd = MPASS;
char *sql_db = MDB;
char *sql_socket = MSOCKET;
int  sql_port = MPORT;

static error_t
mu_sql_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_SQL_GETPWNAM:
      sql_getpwnam_query = arg;
      break;

    case ARG_SQL_GETPWUID:
      sql_getpwuid_query = arg;
      break;

    case ARG_SQL_GETPASS:
      sql_getpass_query = arg;
      break;

    case ARG_SQL_HOST:
      sql_host = arg;
      break;

    case ARG_SQL_USER:
      sql_user = arg;
      break;

    case ARG_SQL_PASSWD:
      sql_passwd = arg;
      break;

    case ARG_SQL_DB:
      sql_db = arg;
      break;

    case ARG_SQL_PORT:
      sql_port = strtoul (arg, NULL, 0);
      if (sql_port == 0)
	{
	  sql_host = NULL;
	  sql_socket = arg;
	}
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_sql_argp = {
  mu_sql_argp_option,
  mu_sql_argp_parser,
};

#endif

struct mu_auth_module mu_auth_sql_module = {
  "sql",
#ifdef HAVE_MYSQL
  &mu_sql_argp,
#else
  NULL,
#endif
  mu_auth_nosupport,
  NULL,
  mu_auth_sql_by_name,
  NULL,
  mu_auth_sql_by_uid,
  NULL
};

