/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.

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
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/sql.h>

#ifdef USE_SQL

int sql_interface = 0;
char *mu_sql_getpwnam_query;
char *mu_sql_getpass_query;
char *mu_sql_getpwuid_query;

char *mu_sql_host = NULL;          /* Hostname to connect to. NULL for UNIX
				      socket connection */
char *mu_sql_user = "accounts";    /* Username for mysql access */    
char *mu_sql_passwd = "yurpass";   /* Password for mysql access */
char *mu_sql_db = "accounts";      /* Database Name */
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

# define ARG_SQL_INTERFACE 256
# define ARG_SQL_GETPWNAM  257
# define ARG_SQL_GETPWUID  258
# define ARG_SQL_GETPASS   259
# define ARG_SQL_HOST      260
# define ARG_SQL_USER      261
# define ARG_SQL_PASSWD    262
# define ARG_SQL_DB        263
# define ARG_SQL_PORT      264

static struct argp_option mu_sql_argp_option[] = {
  {"sql-interface", ARG_SQL_INTERFACE, N_("NAME"), 0,
   N_("Type of SQL interface to use"), 0},
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
    case ARG_SQL_INTERFACE:
      sql_interface = mu_sql_interface_index (arg);
      if (sql_interface == 0)
	argp_error (state, _("Unknown SQL interface '%s'"), arg);
      break;
      
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

static int
mu_auth_sql_by_name (struct mu_auth_data **return_data,
		     const void *key,
		     void *func_data ARG_UNUSED,
		     void *call_data ARG_UNUSED)
{
  int status, rc;
  char *query_str = NULL;
  mu_sql_connection_t conn;
  size_t n;
  
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

  query_str = mu_sql_expand_query (mu_sql_getpwnam_query, key);

  if (!query_str)
    return 1;

  status = mu_sql_connection_init (&conn,
				   sql_interface,
				   mu_sql_host,
				   mu_sql_port,
				   mu_sql_user, mu_sql_passwd,
				   mu_sql_db);

  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return status;
    }

  status = mu_sql_connect (conn);

  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return status;
    }
  
  status = mu_sql_query (conn, query_str);
  free (query_str);
  
  if (status)
    {
      mu_error (_("SQL Query failed: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return 1;
    }

  status = mu_sql_store_result (conn);

  if (status)
    {
      mu_error (_("cannot store SQL result: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return 1;
    }

  mu_sql_num_tuples (conn, &n);
  if (n == 0)
    {
      rc = 1;
    }
  else
    {
      char *mailbox_name;
      char *name;
      
      mu_sql_get_column (conn, 0, 0, &name);

      if (n == 7)
	{
	  char *tmp;
	  mu_sql_get_column (conn, 0, 6, &tmp);
	  mailbox_name = strdup (tmp);
	}
      else
	{
	  mailbox_name = malloc (strlen (mu_path_maildir) +
				 strlen (name) + 1);
	  if (mailbox_name)
	    sprintf (mailbox_name, "%s%s", mu_path_maildir, name);
	}
      
      if (mailbox_name)
	{
	  char *passwd, *suid, *sgid, *dir, *shell;
	  
	  mu_sql_get_column (conn, 0, 1, &passwd);
	  mu_sql_get_column (conn, 0, 2, &suid);
	  mu_sql_get_column (conn, 0, 3, &sgid);
	  mu_sql_get_column (conn, 0, 4, &dir);
	  mu_sql_get_column (conn, 0, 5, &shell);
	  
	  rc = mu_auth_data_alloc (return_data,
				   name,
				   passwd,
				   atoi (suid),
				   atoi (sgid),
				   "SQL User",
				   dir,
				   shell,
				   mailbox_name,
				   1);
	}
      else
	rc = 1;
      
      free (mailbox_name);
    }
  
  mu_sql_release_result (conn);
  mu_sql_disconnect (conn);
  mu_sql_connection_destroy (&conn);
  
  return rc;
}

static int
mu_auth_sql_by_uid (struct mu_auth_data **return_data,
		    const void *key,
		    void *func_data ARG_UNUSED,
		    void *call_data ARG_UNUSED)
{
  char uidstr[64];
  int status, rc;
  char *query_str = NULL;
  mu_sql_connection_t conn;
  size_t n;
  
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

  snprintf (uidstr, sizeof (uidstr), "%u", *(uid_t*)key);
  query_str = mu_sql_expand_query (mu_sql_getpwuid_query, uidstr);

  if (!query_str)
    return 1;

  status = mu_sql_connection_init (&conn,
				   sql_interface,
				   mu_sql_host,
				   mu_sql_port,
				   mu_sql_user, mu_sql_passwd,
				   mu_sql_db);

  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return status;
    }

  status = mu_sql_connect (conn);

  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return status;
    }
  
  status = mu_sql_query (conn, query_str);
  free (query_str);
  
  if (status)
    {
      mu_error (_("SQL Query failed: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return 1;
    }

  status = mu_sql_store_result (conn);

  if (status)
    {
      mu_error (_("cannot store SQL result: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return 1;
    }

  mu_sql_num_tuples (conn, &n);

  if (n == 0)
    {
      rc = 1;
    }
  else
    {
      char *name;
      char *mailbox_name;
      
      mu_sql_get_column (conn, 0, 0, &name);
  
      if (n == 7)
	{
	  char *tmp;
	  mu_sql_get_column (conn, 0, 6, &tmp);
	  mailbox_name = strdup (tmp);
	}
      else
	{
	  mailbox_name = malloc (strlen (mu_path_maildir) +
				 strlen (name) + 1);
	  if (mailbox_name)
	    sprintf (mailbox_name, "%s%s", mu_path_maildir, name);
	}
      
      if (mailbox_name)
	{
	  char *passwd, *suid, *sgid, *dir, *shell;
	  
	  mu_sql_get_column (conn, 0, 1, &passwd);
	  mu_sql_get_column (conn, 0, 2, &suid);
	  mu_sql_get_column (conn, 0, 3, &sgid);
	  mu_sql_get_column (conn, 0, 4, &dir);
	  mu_sql_get_column (conn, 0, 5, &shell);
	  
	  rc = mu_auth_data_alloc (return_data,
				   name,
				   passwd,
				   atoi (suid),
				   atoi (sgid),
				   "SQL User",
				   dir,
				   shell,
				   mailbox_name,
				   1);
	}
      else
	rc = 1;
      free (mailbox_name);
    }
  
  mu_sql_release_result (conn);
  mu_sql_disconnect (conn);
  mu_sql_connection_destroy (&conn);
  
  return rc;
}

static int
mu_sql_authenticate (struct mu_auth_data **return_data ARG_UNUSED,
		     const void *key,
		     void *func_data ARG_UNUSED, void *call_data)
{
  mu_sql_connection_t conn;
  struct mu_auth_data *auth_data = key;
  char *pass = call_data;
  char *sql_pass;
  char *query_str = NULL;
  int rc, status;
  
  if (!auth_data)
    return 1;

  query_str = mu_sql_expand_query (mu_sql_getpass_query, auth_data->name);

  if (!query_str)
    return 1;

  status = mu_sql_connection_init (&conn,
				   sql_interface,
				   mu_sql_host,
				   mu_sql_port,
				   mu_sql_user, mu_sql_passwd,
				   mu_sql_db);

  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return status;
    }

  status = mu_sql_connect (conn);

  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return status;
    }
  
  status = mu_sql_query (conn, query_str);
  free (query_str);
  
  if (status)
    {
      mu_error (_("SQL Query failed: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return 1;
    }

  status = mu_sql_store_result (conn);

  if (status)
    {
      mu_error (_("cannot store SQL result: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return 1;
    }

  status = mu_sql_get_column (conn, 0, 0, &sql_pass);
  if (status)
    {
      mu_error (_("cannot get password from SQL: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_release_result (conn);
      mu_sql_connection_destroy (&conn);
      return 1;
    }
  
  rc = strcmp (sql_pass, crypt (pass, sql_pass));

  mu_sql_disconnect (conn);
  mu_sql_connection_destroy (&conn);
  
  return rc;
}

#else

# define mu_sql_authenticate mu_auth_nosupport
# define mu_auth_sql_by_name mu_auth_nosupport
# define mu_auth_sql_by_uid mu_auth_nosupport

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

