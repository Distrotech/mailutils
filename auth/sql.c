/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.

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
#else
# include <string.h>
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
#include <mailutils/nls.h>

#ifdef USE_SQL

char *mu_sql_getpwnam_query;
char *mu_sql_getpass_query;
char *mu_sql_getpwuid_query;

char *mu_sql_host = NULL;          /* Hostname to connect to. NULL for UNIX
                                   socket connection */
char *mu_sql_user = "accounts";    /* Username for mysql access */    
char *mu_sql_passwd = "yurpass";   /* Password for mysql access */
char *mu_sql_db = "accounts";      /* Database Name */
char *mu_sql_socket = NULL;        /* Socket name to use. Valid only if
				   connecting via UNIX sockets */
int  mu_sql_port = 0;              /* Port number to connect to.
				   0 means default port */

char *
mu_sql_expand_query (const char *query, const char *ustr)
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

# define ARG_SQL_GETPWNAM 1
# define ARG_SQL_GETPWUID 2
# define ARG_SQL_GETPASS 3
# define ARG_SQL_HOST 4
# define ARG_SQL_USER 5
# define ARG_SQL_PASSWD 6
# define ARG_SQL_DB 7
# define ARG_SQL_PORT 8

static struct argp_option mu_sql_argp_option[] = {
  {"sql-getpwnam", ARG_SQL_GETPWNAM, N_("QUERY"), 0,
   N_("SQL query to retrieve a passwd entry based on username"), 0},
  {"sql-getpwuid", ARG_SQL_GETPWUID, N_("QUERY"), 0,
   N_("SQL query to retrieve a passwd entry based on UID"), 0},
  {"sql-getpass", ARG_SQL_GETPASS, N_("QUERY"), 0,
   N_("SQL query to retrieve a password from the database"), 0},
  {"sql-host", ARG_SQL_HOST, N_("HOSTNAME"), 0,
   N_("Name or IP of MySQL server to connect to"), 0},
  {"sql-user", ARG_SQL_USER, N_("NAME"), 0,
   N_("SQL user name"), 0},
  {"sql-passwd", ARG_SQL_PASSWD, N_("STRING"), 0,
   N_("SQL connection password"), 0},
  {"sql-db", ARG_SQL_DB, N_("STRING"), 0,
   N_("Name of the database to connect to"), 0},
  {"sql-port", ARG_SQL_PORT, N_("NUMBER"), 0,
   N_("Port to use"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_sql_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_SQL_GETPWNAM:
      mu_sql_getpwnam_query = arg;
      break;

    case ARG_SQL_GETPWUID:
      mu_sql_getpwuid_query = arg;
      break;

    case ARG_SQL_GETPASS:
      mu_sql_getpass_query = arg;
      break;

    case ARG_SQL_HOST:
      mu_sql_host = arg;
      break;

    case ARG_SQL_USER:
      mu_sql_user = arg;
      break;

    case ARG_SQL_PASSWD:
      mu_sql_passwd = arg;
      break;

    case ARG_SQL_DB:
      mu_sql_db = arg;
      break;

    case ARG_SQL_PORT:
      mu_sql_port = strtoul (arg, NULL, 0);
      if (mu_sql_port == 0)
	{
	  mu_sql_host = NULL;
	  mu_sql_socket = arg;
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


# ifdef HAVE_MYSQL
int mysql_auth_sql_by_name __P((struct mu_auth_data **return_data, void *key,
				void *func_data, void *call_data));
int mysql_auth_sql_by_uid __P((struct mu_auth_data **return_data, void *key,
			       void *func_data, void *call_data));
int mysql_sql_authenticate __P((struct mu_auth_data **return_data, void *key,
				void *func_data, void *call_data));

#  define mu_sql_authenticate mysql_sql_authenticate
#  define mu_auth_sql_by_name mysql_auth_sql_by_name
#  define mu_auth_sql_by_uid  mysql_auth_sql_by_uid

# endif

# ifdef HAVE_PGSQL
int pg_auth_sql_by_name __P((struct mu_auth_data **return_data, void *key,
			     void *func_data, void *call_data));
int pg_auth_sql_by_uid __P((struct mu_auth_data **return_data, void *key,
			    void *func_data, void *call_data));
int pg_sql_authenticate __P((struct mu_auth_data **return_data, void *key,
			     void *func_data, void *call_data));

#  define mu_sql_authenticate pg_sql_authenticate
#  define mu_auth_sql_by_name pg_auth_sql_by_name
#  define mu_auth_sql_by_uid  pg_auth_sql_by_uid

# endif

#else

# define mu_sql_authenticate mu_auth_nosupport
# define mu_auth_sql_by_name mu_auth_nosupport
# define mu_auth_sql_by_uid  mu_auth_nosupport

#endif

struct mu_auth_module mu_auth_sql_module = {
  "sql",
#ifdef USE_SQL
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

