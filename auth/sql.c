/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2003, 2004, 2005, 2006,
   2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

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

#include <mailutils/assoc.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/argp.h>
#include <mailutils/mu_auth.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/mutil.h>
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
enum mu_password_type mu_sql_password_type  = password_hash;

static mu_assoc_t sql_field_map;

static char *
sql_escape_string (const char *ustr)
{
  char *str, *q;
  const unsigned char *p;
  size_t len = strlen (ustr);
#define ESCAPABLE_CHAR "\\'\""
  
  for (p = (const unsigned char *) ustr; *p; p++)
    {
      if (strchr (ESCAPABLE_CHAR, *p))
	len++;
    }

  str = malloc (len + 1);
  if (!str)
    return NULL;

  for (p = (const unsigned char *) ustr, q = str; *p; p++)
    {
      if (strchr (ESCAPABLE_CHAR, *p))
	*q++ = '\\';
      *q++ = *p;
    }
  *q = 0;
  return str;
}

char *
mu_sql_expand_query (const char *query, const char *ustr)
{
  char *p, *q, *res;
  int len;
  char *esc_ustr;
  
  if (!query)
    return NULL;

  esc_ustr = sql_escape_string (ustr);
  
  /* Compute resulting query length */
  for (len = 0, p = (char *) query; *p; )
    {
      if (*p == '%')
	{
	  if (p[1] == 'u')
	    {
	      len += strlen (esc_ustr);
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
    {
      free (esc_ustr);
      return res;
    }

  for (p = (char *) query, q = res; *p; )
    {
      if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'u':
	      strcpy (q, esc_ustr);
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

  free (esc_ustr);
  return res;
}

# define ARG_SQL_INTERFACE        256
# define ARG_SQL_GETPWNAM         257
# define ARG_SQL_GETPWUID         258
# define ARG_SQL_GETPASS          259
# define ARG_SQL_HOST             260
# define ARG_SQL_USER             261
# define ARG_SQL_PASSWD           262
# define ARG_SQL_DB               263
# define ARG_SQL_PORT             264
# define ARG_SQL_MU_PASSWORD_TYPE 265
# define ARG_SQL_FIELD_MAP        266

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
  {"sql-password-type", ARG_SQL_MU_PASSWORD_TYPE, N_("STRING"), 0,
   N_("Type of password returned by --sql-getpass query. STRING is one of: plain, hash, scrambled"), 0},
  {"sql-field-map", ARG_SQL_FIELD_MAP, N_("MAP"), 0,
   N_("Declare a name translation map for SQL fields in results of sql-getpwnam and "
      "sql-getpwuid queries"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_sql_argp_parser (int key, char *arg, struct argp_state *state)
{
  int rc, err;
  
  switch (key)
    {
    case ARG_SQL_INTERFACE:
      sql_interface = mu_sql_interface_index (arg);
      if (sql_interface == 0)
	argp_error (state, _("Unknown SQL interface `%s'"), arg);
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

    case ARG_SQL_MU_PASSWORD_TYPE:
      if (strcmp (arg, "plain") == 0)
	mu_sql_password_type  = password_plaintext;
      else if (strcmp (arg, "hash") == 0)
	mu_sql_password_type  = password_hash;
      else if (strcmp (arg, "scrambled") == 0)
	mu_sql_password_type  = password_scrambled;
      else
	argp_error (state, _("Unknown password type `%s'"), arg);
      break;

    case ARG_SQL_FIELD_MAP:
      rc = mutil_parse_field_map (arg, &sql_field_map, &err);
      if (rc)
	argp_error (state, _("Error near element %d: %s"),
		    err, mu_strerror (rc));
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
decode_tuple_v1_0 (mu_sql_connection_t conn, int n,
		   struct mu_auth_data **return_data)
{
  int rc;
  char *mailbox_name = NULL;
  char *name;
      
  if (mu_sql_get_column (conn, 0, 0, &name))
    return MU_ERR_FAILURE;

  if (n == 7)
    {
      char *tmp;
      if (mu_sql_get_column (conn, 0, 6, &tmp))
	return MU_ERR_FAILURE;
      if ((mailbox_name = strdup (tmp)) == NULL)
	return ENOMEM;
    }
  else if (mu_construct_user_mailbox_url (&mailbox_name, name))
    return MU_ERR_FAILURE;
      
  if (mailbox_name)
    {
      char *passwd, *suid, *sgid, *dir, *shell;
	  
      if (mu_sql_get_column (conn, 0, 1, &passwd)
	  || !passwd
	  || mu_sql_get_column (conn, 0, 2, &suid)
	  || !suid
	  || mu_sql_get_column (conn, 0, 3, &sgid)
	  || !sgid
	  || mu_sql_get_column (conn, 0, 4, &dir)
	  || !dir
	  || mu_sql_get_column (conn, 0, 5, &shell)
	  || !shell)
	return MU_ERR_FAILURE;
      
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
    rc = MU_ERR_AUTH_FAILURE;
  
  free (mailbox_name);
  return rc;
}

static int
get_field (mu_sql_connection_t conn, const char *id, char **ret, int mandatory)
{
  const char **name = mu_assoc_ref (sql_field_map, id);
  int rc = mu_sql_get_field (conn, 0, name ? *name : id, ret);
  if (rc)
    {
      if (mandatory || rc != MU_ERR_NOENT)
	mu_error (_("Cannot get SQL field `%s' (`%s'): %s"),
		  id, name ? *name : id, mu_strerror (rc));
    }
  else if (!*ret)
    {
      mu_error (_("SQL field `%s' (`%s') has NULL value"),
		id, name ? *name : id);
      rc = MU_READ_ERROR;
    }

  return rc;
}

static int
decode_tuple_new (mu_sql_connection_t conn, int n,
		  struct mu_auth_data **return_data)
{
  int rc;
  char *mailbox_name = NULL;
  char *name;
  char *passwd, *suid, *sgid, *dir, *shell, *gecos, *squota;
  mu_off_t quota = 0;
  char *p;
  uid_t uid;
  gid_t gid;

  if (get_field (conn, MU_AUTH_NAME, &name, 1)
      || get_field (conn, MU_AUTH_PASSWD, &passwd, 1)
      || get_field (conn, MU_AUTH_UID, &suid, 1)
      || get_field (conn, MU_AUTH_GID, &sgid, 1)     
      || get_field (conn, MU_AUTH_DIR, &dir, 1)     
      || get_field (conn, MU_AUTH_SHELL, &shell, 1))
    return MU_ERR_FAILURE;

  if (get_field (conn, MU_AUTH_GECOS, &gecos, 0))
    gecos = "SQL user";
  
  uid = strtoul (suid, &p, 0);
  if (*p)
    {
      mu_error (_("Invalid value for uid: %s"), suid);
      return MU_ERR_FAILURE;
    }

  gid = strtoul (sgid, &p, 0);
  if (*p)
    {
      mu_error (_("Invalid value for gid: %s"), sgid);
      return MU_ERR_FAILURE;
    }
  
  rc = get_field (conn, MU_AUTH_MAILBOX, &mailbox_name, 0);
  switch (rc)
    {
    case 0:
      mailbox_name = strdup (mailbox_name);
      break;
      
    case MU_ERR_NOENT:
      if (mu_construct_user_mailbox_url (&mailbox_name, name))
	return MU_ERR_FAILURE;
      break;

    default:
      return MU_ERR_FAILURE;
    }

  rc = get_field (conn, MU_AUTH_QUOTA, &squota, 0);
  if (rc == 0)
    {
      if (strcasecmp (squota, "none") == 0)
	quota = 0;
      else
	{
	  quota = strtoul (squota, &p, 10);
	  switch (*p)
	    {
	    case 0:
	      break;
	      
	    case 'k':
	    case 'K':
	      quota *= 1024;
	      break;
      
	    case 'm':
	    case 'M':
	      quota *= 1024*1024;
	      break;
	      
	    default:
	      mu_error (_("Invalid value for quota: %s"), squota);
	      free (mailbox_name);
	      return MU_ERR_FAILURE;
	    }
	}
    }
  else if (rc ==  MU_ERR_NOENT)
    quota = 0;
  else
    {
      free (mailbox_name);
      return MU_ERR_FAILURE;
    }

  rc = mu_auth_data_alloc (return_data,
			   name,
			   passwd,
			   uid,
			   gid,
			   gecos,
			   dir,
			   shell,
			   mailbox_name,
			   1);
  if (rc == 0)
    mu_auth_data_set_quota (*return_data, quota);
  
  free (mailbox_name);
  return rc;
}  

static int
decode_tuple (mu_sql_connection_t conn, int n,
	      struct mu_auth_data **return_data)
{
  if (sql_field_map)
    return decode_tuple_new (conn, n, return_data);
  else
    return decode_tuple_v1_0 (conn, n, return_data);
}
 
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
    return EINVAL;

  query_str = mu_sql_expand_query (mu_sql_getpwnam_query, key);

  if (!query_str)
    return MU_ERR_FAILURE;

  status = mu_sql_connection_init (&conn,
				   sql_interface,
				   mu_sql_host,
				   mu_sql_port,
				   mu_sql_user, mu_sql_passwd,
				   mu_sql_db);

  if (status)
    {
      mu_error ("%s: %s", mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return MU_ERR_FAILURE;
    }

  status = mu_sql_connect (conn);

  if (status)
    {
      mu_error ("%s: %s", mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return EAGAIN;
    }
  
  status = mu_sql_query (conn, query_str);
  free (query_str);
  
  if (status)
    {
      mu_error (_("SQL Query failed: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return MU_ERR_FAILURE;
    }

  status = mu_sql_store_result (conn);

  if (status)
    {
      mu_error (_("Cannot store SQL result: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return MU_ERR_FAILURE;
    }

  mu_sql_num_tuples (conn, &n);
  if (n == 0)
    rc = MU_ERR_AUTH_FAILURE;
  else
    rc = decode_tuple (conn, n, return_data);
  
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
    return EINVAL;

  snprintf (uidstr, sizeof (uidstr), "%u", *(uid_t*)key);
  query_str = mu_sql_expand_query (mu_sql_getpwuid_query, uidstr);

  if (!query_str)
    return ENOMEM;

  status = mu_sql_connection_init (&conn,
				   sql_interface,
				   mu_sql_host,
				   mu_sql_port,
				   mu_sql_user, mu_sql_passwd,
				   mu_sql_db);

  if (status)
    {
      mu_error ("%s: %s", mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return MU_ERR_FAILURE;
    }

  status = mu_sql_connect (conn);

  if (status)
    {
      mu_error ("%s: %s", mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return EAGAIN;
    }
  
  status = mu_sql_query (conn, query_str);
  free (query_str);
  
  if (status)
    {
      mu_error (_("SQL Query failed: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return MU_ERR_FAILURE;
    }

  status = mu_sql_store_result (conn);

  if (status)
    {
      mu_error (_("Cannot store SQL result: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return MU_ERR_FAILURE;
    }

  mu_sql_num_tuples (conn, &n);

  if (n == 0)
    rc = MU_ERR_AUTH_FAILURE;
  else
    rc = decode_tuple (conn, n, return_data);
  
  mu_sql_release_result (conn);
  mu_sql_disconnect (conn);
  mu_sql_connection_destroy (&conn);
  
  return rc;
}

int
mu_sql_getpass (const char *username, char **passwd)
{
  mu_sql_connection_t conn;
  char *query_str;
  int status;
  char *sql_pass;
  
  query_str = mu_sql_expand_query (mu_sql_getpass_query, username);

  if (!query_str)
    return MU_ERR_FAILURE;

  status = mu_sql_connection_init (&conn,
				   sql_interface,
				   mu_sql_host,
				   mu_sql_port,
				   mu_sql_user, mu_sql_passwd,
				   mu_sql_db);

  if (status)
    {
      mu_error ("%s: %s", mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return MU_ERR_FAILURE;
    }

  status = mu_sql_connect (conn);

  if (status)
    {
      mu_error ("%s: %s", mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return EAGAIN;
    }
  
  status = mu_sql_query (conn, query_str);
  free (query_str);
  
  if (status)
    {
      mu_error (_("SQL Query failed: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return MU_ERR_FAILURE;
    }

  status = mu_sql_store_result (conn);

  if (status)
    {
      mu_error (_("Cannot store SQL result: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return MU_ERR_FAILURE;
    }

  status = mu_sql_get_column (conn, 0, 0, &sql_pass);
  if (status)
    {
      mu_error (_("Cannot get password from SQL: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_release_result (conn);
      mu_sql_connection_destroy (&conn);
      return MU_ERR_FAILURE;
    }

  *passwd = strdup (sql_pass);

  mu_sql_disconnect (conn);
  mu_sql_connection_destroy (&conn);

  if (!*passwd)
    return ENOMEM;

  return 0;
}

static int
mu_sql_authenticate (struct mu_auth_data **return_data ARG_UNUSED,
		     const void *key,
		     void *func_data ARG_UNUSED, void *call_data)
{
  const struct mu_auth_data *auth_data = key;
  char *pass = call_data;
  char *sql_pass;
  int rc;
  
  if (!auth_data)
    return EINVAL;

  if ((rc = mu_sql_getpass (auth_data->name, &sql_pass)))
    return rc;

  switch (mu_sql_password_type)
    {
    case password_hash:
      rc = strcmp (sql_pass, crypt (pass, sql_pass));
      break;

    case password_scrambled:
      /* FIXME: Should this call be implementation-independent? I mean,
         should we have mu_sql_check_scrambled() that will match the
	 password depending on the exact type of the underlying database,
	 just as the rest of mu_sql_.* functions do */
#ifdef HAVE_MYSQL
      rc = mu_check_mysql_scrambled_password (sql_pass, pass);
#else
      rc = 1;
#endif
      break;

    case password_plaintext:
      rc = strcmp (sql_pass, pass);
      break;
    }

  free (sql_pass);
  
  return rc == 0 ? 0 : MU_ERR_AUTH_FAILURE;
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

