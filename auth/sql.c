/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
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
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/argp.h>
#include <mailutils/mu_auth.h>
#include <mailutils/error.h>

#ifdef HAVE_MYSQL
#include <mysql/mysql.h>

#define MFLAGS 0                /* Special user flags. It is safe to leave
				   this untouched */

char *sql_getpwnam_query;
char *sql_getpass_query;
char *sql_getpwuid_query;

char *sql_host = NULL;          /* Hostname to connect to. NULL for UNIX
                                   socket connection */
char *sql_user = "accounts";    /* Username for mysql access */    
char *sql_passwd = "yurpass";   /* Password for mysql access */
char *sql_db = "accounts";      /* Database Name */
char *sql_socket = NULL;        /* Socket name to use. Valid only if
				   connecting via UNIX sockets */
int  sql_port = 0;              /* Port number to connect to. 0 means default
	                           MySQL port (3300) */

static char *
sql_expand_query (const char *query, const char *ustr)
{
  char *p, *q, *res;
  int len;

  if (!query)
    return NULL;
  
  /* Compute resulting query length */
  for (len = 0, p = (char *) query; *p; )
    {
      if (*p == '%')
	{
	  if (p[1] == 'u')
	    {
	      len += strlen (ustr);
	      p += 2;
	    }
	  else if (p[1] == '%')
	    {
	      len++;
	      p += 2;
	    }
	  else
	    {
	      len++;
	      p++;
	    }
	}
      else
	{
	  len++;
	  p++;
	}
    }

  res = malloc (len + 1);
  if (!res)
    return res;

  for (p = (char *) query, q = res; *p; )
    {
      if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'u':
	      strcpy (q, ustr);
	      q += strlen (q);
	      p++;
	      break;
	      
	    case '%':
	      *q++ = *p++;
	      break;
	      
	    default:
	      *q++ = *p++;
	    }
	}
      else
	*q++ = *p++;
    }
  *q = 0;
  return res;
}

static int
mu_auth_sql_by_name (void *return_data, void *key,
		     void *unused_func_data, void *unused_call_data)
{
  char *query_str = NULL;
  MYSQL *m;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *mailbox_name;
  int rc;
  
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

  m = mysql_init (0);

  if (!m)
    return 1;

  if (!mysql_real_connect (m, sql_host, sql_user, sql_passwd, sql_db, sql_port,
			   sql_socket, MFLAGS))
    {
      mu_error ("MySQL: connect failed: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }

  query_str = sql_expand_query (sql_getpwnam_query, key);

  if (!query_str)
    {
      mysql_close (m);
      return 1;
    }
  
  if (mysql_query (m, query_str) != 0)
    {
      mu_error ("MySQL: query failed: %s", mysql_error (m));
      free (query_str);
      mysql_close (m);
      return 1;
    }

  free (query_str);

  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error ("MySQL: can't store result: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error ("MySQL: can't fetch row: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }

  if (mysql_num_fields (res) == 7 && row[6])
    mailbox_name = strdup (row[6]);
  else
    {
      mailbox_name = malloc (strlen (mu_path_maildir) +
			     strlen (row[0]) + 1);
      if (mailbox_name)
	sprintf (mailbox_name, "%s%s", mu_path_maildir, row[0]);
    }

  if (mailbox_name)
    rc = mu_auth_data_alloc ((struct mu_auth_data **) return_data,
			     row[0],
			     row[1],
			     atoi (row[2]),
			     atoi (row[3]),
			     "Mysql User",
			     row[4],
			     row[5],
			     mailbox_name,
			     1);
  else
    rc = 1;
  
  free (mailbox_name);
  mysql_free_result (res);
  mysql_close (m);
  
  return rc;
}

static int
mu_auth_sql_by_uid (void *return_data, void *key,
		    void *unused_func_data, void *unused_call_data)
{
  char *query_str = NULL;
  MYSQL *m;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char uidstr[64];
  char *mailbox_name;
  int rc;
  
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

  m = mysql_init (0);

  if (!m)
    return 1;

  if (!mysql_real_connect (m, sql_host, sql_user, sql_passwd, sql_db, sql_port,
			   sql_socket, MFLAGS))
    {
      mu_error ("MySQL: connect failed: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }

  snprintf (uidstr, sizeof (uidstr), "%u", *(uid_t*)key);
  query_str = sql_expand_query (sql_getpwnam_query, uidstr);

  if (!query_str)
    {
      mysql_close (m);
      return 1;
    }
  
  if (mysql_query (m, query_str) != 0)
    {
      mu_error ("MySQL: query failed: %s", mysql_error (m));
      free (query_str);
      mysql_close (m);
      return 1;
    }

  free (query_str);
  
  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error ("MySQL: can't store result: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error ("MySQL: can't fetch row: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }

  if (mysql_num_fields (res) == 7 && row[6])
    mailbox_name = strdup (row[6]);
  else
    {
      mailbox_name = malloc (strlen (mu_path_maildir) +
			     strlen (row[0]) + 1);
      if (mailbox_name)
	sprintf (mailbox_name, "%s%s", mu_path_maildir, row[0]);
    }

  if (mailbox_name)
    rc = mu_auth_data_alloc ((struct mu_auth_data **) return_data,
			     row[0],
			     row[1],
			     atoi (row[2]),
			     atoi (row[3]),
			     "Mysql User",
			     row[4],
			     row[5],
			     mailbox_name,
			     1);
  else
    rc = 1;
  
  free (mailbox_name);
  mysql_free_result (res);
  mysql_close (m);
  
  return rc;
}

static int
mu_sql_authenticate (void *ignored_return_data, void *key,
		     void *ignored_func_data, void *call_data)
{
  struct mu_auth_data *auth_data = key;
  char *query_str = NULL;
  MYSQL *m;
  MYSQL_RES *res;
  MYSQL_ROW row;
  int rc;
  
  if (!auth_data)
    return 1;

  m = mysql_init (0);

  if (!m)
    return 1;
  
  if (!mysql_real_connect (m, sql_host, sql_user, sql_passwd, sql_db, sql_port,
			   sql_socket, MFLAGS))
    {
      mu_error ("MySQL: connect failed: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  query_str = sql_expand_query (sql_getpass_query, auth_data->name);

  if (!query_str)
    {
      mysql_close (m);
      return 1;
    }

  if (mysql_query (m, query_str) != 0)
    {
      mu_error ("MySQL: query failed: %s", mysql_error (m));
      free (query_str);
      mysql_close (m);
      return 1;
    }

  free (query_str);
  
  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error ("MySQL: can't store result: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error ("MySQL: can't fetch row: %s", mysql_error (m));
      mysql_close (m);
      return 1;
    }

  rc = strcmp (row[0], crypt (auth_data->passwd, row[0]));
  mysql_free_result (res);
  mysql_close (m);
  return rc;
}


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

#else

#define mu_sql_authenticate mu_auth_nosupport
#define mu_auth_sql_by_name mu_auth_nosupport
#define mu_auth_sql_by_uid  mu_auth_nosupport

#endif

struct mu_auth_module mu_auth_sql_module = {
  "sql",
#ifdef HAVE_MYSQL
  &mu_sql_argp,
#else
  NULL,
#endif
  mu_sql_authenticate,
  NULL,
  mu_auth_sql_by_name,
  NULL,
  mu_auth_sql_by_uid,
  NULL
};

