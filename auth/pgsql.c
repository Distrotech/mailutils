/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#ifdef HAVE_PGSQL
#include <sql.h>
#include <libpq-fe.h>

static PGconn *
pg_connect ()
{
  PGconn  *pgconn;
  char portbuf[16];
  char *portstr;
        
  if (mu_sql_port == 0)
    portstr = NULL;
  else
    {
      portstr = portbuf;
      snprintf (portbuf, sizeof (portbuf), "%d", mu_sql_port);
    }
                
  pgconn = PQsetdbLogin (mu_sql_host, portstr, NULL, NULL,
			 mu_sql_db, mu_sql_user, mu_sql_passwd);
        
  if (PQstatus (pgconn) == CONNECTION_BAD)
    {
      mu_error("PQconnectStart: %s", PQerrorMessage (pgconn));
      PQfinish (pgconn);
      return NULL;
    }
  return pgconn;
}

static char *
chop (char *str)
{
  int len;
  
  for (len = strlen(str); len > 0 && isspace(str[len-1]); len--)
    ;
  str[len] = 0;
  return str;
}

static int
pg_auth_common (PGresult *res, char *query_str, struct mu_auth_data **auth)
{
  char *user_name, *passwd, *homedir, *shell, *mailbox_name;
  uid_t uid;
  gid_t gid;
  ExecStatusType stat;
  int ntuples, nfields;
  int rc = 1;
  
  stat = PQresultStatus (res);

  if (stat != PGRES_TUPLES_OK)
    {
      mu_error (_("PQexec status: %s"), PQresStatus(stat));
      return 1;
    }

  ntuples = PQntuples (res);
  nfields = PQnfields (res);
  if (ntuples > 1 && nfields)
    {
      mu_error (ngettext("query returned %d tuple: %s",
			 "query returned %d tuples: %s",
			 ntuples),
		ntuples, query_str);
    }
  
  if (nfields < 6)
    {
      mu_error (ngettext("query returned %d field: %s",
			 "query returned %d fields: %s",
			 nfields),
		nfields, query_str);
      return 1;
    }

  user_name = chop (PQgetvalue (res, 0, 0));
  passwd = chop (PQgetvalue (res, 0, 1));
  uid = strtoul (PQgetvalue (res, 0, 2), NULL, 0);
  gid = strtoul (PQgetvalue (res, 0, 3), NULL, 0);
  homedir = chop (PQgetvalue (res, 0, 4));
  shell = chop (PQgetvalue (res, 0, 5));
  
  if (ntuples == 7)
    {
      mailbox_name = strdup (chop (PQgetvalue (res, 0, 6)));
    }
  else
    {
      mailbox_name = malloc (strlen (mu_path_maildir) +
			     strlen (user_name) + 1);
      if (mailbox_name)
	sprintf (mailbox_name, "%s%s", mu_path_maildir, user_name);
    }
  
  if (mailbox_name)
    rc = mu_auth_data_alloc (auth,
			     user_name,
			     passwd,
			     uid,
			     gid,
			     "PGsql User",
			     homedir,
			     shell,
			     mailbox_name,
			     1);
  free (mailbox_name);
  return rc;
}

int
pg_auth_sql_by_name (void *return_data, void *key,
		     void *func_data ARG_UNUSED,
		     void *call_data ARG_UNUSED)
{
  char *query_str = NULL;
  int rc;
  PGconn *conn;
  PGresult *res = NULL;
  
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

  conn = pg_connect ();
  if (!conn)
    {
      /* Error message already issued */
      return 1;
    }

  query_str = mu_sql_expand_query (mu_sql_getpwnam_query, key);

  if (!query_str)
    {
      PQfinish (conn);
      return 1;
    }

  res = PQexec (conn, query_str);
  if (res == NULL)
    {
      mu_error ("PQexec: %s", PQerrorMessage (conn));
      rc = 1;
    }
  else
    {
      rc = pg_auth_common (res, query_str,
			   (struct mu_auth_data **)return_data);
      PQclear(res);
    }

  free (query_str);
  PQfinish (conn);
  
  return rc;
}

int
pg_auth_sql_by_uid (void *return_data, void *key,
		    void *func_data ARG_UNUSED,
		    void *call_data ARG_UNUSED)
{
  char uidstr[64];
  char *query_str = NULL;
  int rc;
  PGconn *conn;
  PGresult *res = NULL;

  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

  conn = pg_connect ();
  if (!conn)
    {
      /* Error message already issued */
      return 1;
    }

  snprintf (uidstr, sizeof (uidstr), "%u", *(uid_t*)key);
  query_str = mu_sql_expand_query (mu_sql_getpwuid_query, uidstr);
  if (!query_str)
    {
      PQfinish (conn);
      return 1;
    }

  res = PQexec (conn, query_str);
  if (res == NULL)
    {
      mu_error ("PQexec: %s", PQerrorMessage (conn));
      rc = 1;
    }
  else
    {
      rc = pg_auth_common (res, query_str,
			   (struct mu_auth_data **)return_data);
      PQclear(res);
    }

  free (query_str);
  PQfinish (conn);
  
  return rc;
}

int
pg_sql_authenticate (void *return_data ARG_UNUSED, void *key,
		     void *func_data ARG_UNUSED, void *call_data)
{
  PGconn *conn;
  PGresult *res = NULL;
  
  struct mu_auth_data *auth_data = key;
  char *pass = call_data;
  char *query_str = NULL;
  int rc = 1;
  
  if (!auth_data)
    return 1;

  conn = pg_connect ();
  if (!conn)
    {
      /* Error message already issued */
      return 1;
    }

  query_str = mu_sql_expand_query (mu_sql_getpass_query, auth_data->name);
  if (!query_str)
    {
      PQfinish (conn);
      return 1;
    }

  res = PQexec (conn, query_str);
  if (res == NULL)
    {
      mu_error ("PQexec: %s", PQerrorMessage (conn));
    }
  else
    {
      ExecStatusType stat = PQresultStatus (res);
      
      if (stat != PGRES_TUPLES_OK)
	mu_error (_("PQexec status: %s"), PQresStatus(stat));
      else
	{
	  char *p;
	  int ntuples = PQntuples (res);
	  int nfields = PQnfields (res);
	  if (ntuples > 1 && nfields)
	    {
	      mu_error (ngettext("query returned %d tuple: %s",
				 "query returned %d tuples: %s",
				 ntuples),
			ntuples, query_str);
	    }
	  
	  if (nfields > 1)
	    {
	      mu_error (ngettext("query returned %d field: %s",
				 "query returned %d fields: %s",
				 nfields),
			nfields, query_str);
	    }
	  
	  p = chop (PQgetvalue (res, 0, 0));
	  rc = strcmp (p, crypt (pass, p));
	}
      PQclear (res);
    }

  free (query_str);
  PQfinish (conn);

  return rc;
}
#endif
