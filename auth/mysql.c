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

#ifdef HAVE_MYSQL

#include <sql.h>
#include <mysql/mysql.h>

#define MFLAGS 0                /* Special user flags. It is safe to leave
				   this untouched */


int
mysql_auth_sql_by_name (void *return_data, void *key,
			void *func_data ARG_UNUSED,
			void *call_data ARG_UNUSED)
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

  if (!mysql_real_connect (m,
			   mu_sql_host, mu_sql_user, mu_sql_passwd,
			   mu_sql_db, mu_sql_port,
			   mu_sql_socket, MFLAGS))
    {
      mu_error (_("MySQL: connect failed: %s"), mysql_error (m));
      mysql_close (m);
      return 1;
    }

  query_str = mu_sql_expand_query (mu_sql_getpwnam_query, key);

  if (!query_str)
    {
      mysql_close (m);
      return 1;
    }
  
  if (mysql_query (m, query_str) != 0)
    {
      mu_error (_("MySQL: query failed: %s"), mysql_error (m));
      free (query_str);
      mysql_close (m);
      return 1;
    }

  free (query_str);

  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error (_("MySQL: can't store result: %s"), mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error (_("MySQL: can't fetch row: %s"), mysql_error (m));
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

int
mysql_auth_sql_by_uid (void *return_data, void *key,
		       void *func_data ARG_UNUSED,
		       void *call_data ARG_UNUSED)
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

  if (!mysql_real_connect (m,
			   mu_sql_host, mu_sql_user,
			   mu_sql_passwd, mu_sql_db,
			   mu_sql_port, mu_sql_socket, MFLAGS))
    {
      mu_error (_("MySQL: connect failed: %s"), mysql_error (m));
      mysql_close (m);
      return 1;
    }

  snprintf (uidstr, sizeof (uidstr), "%u", *(uid_t*)key);
  query_str = mu_sql_expand_query (mu_sql_getpwuid_query, uidstr);

  if (!query_str)
    {
      mysql_close (m);
      return 1;
    }
  
  if (mysql_query (m, query_str) != 0)
    {
      mu_error (_("MySQL: query failed: %s"), mysql_error (m));
      free (query_str);
      mysql_close (m);
      return 1;
    }

  free (query_str);
  
  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error (_("MySQL: can't store result: %s"), mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error (_("MySQL: can't fetch row: %s"), mysql_error (m));
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

int
mysql_sql_authenticate (void *return_data ARG_UNUSED, void *key,
			void *func_data ARG_UNUSED, void *call_data)
{
  struct mu_auth_data *auth_data = key;
  char *pass = call_data;
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
  
  if (!mysql_real_connect (m,
			   mu_sql_host, mu_sql_user,
			   mu_sql_passwd, mu_sql_db,
			   mu_sql_port,
			   mu_sql_socket, MFLAGS))
    {
      mu_error (_("MySQL: connect failed: %s"), mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  query_str = mu_sql_expand_query (mu_sql_getpass_query, auth_data->name);

  if (!query_str)
    {
      mysql_close (m);
      return 1;
    }

  if (mysql_query (m, query_str) != 0)
    {
      mu_error (_("MySQL: query failed: %s"), mysql_error (m));
      free (query_str);
      mysql_close (m);
      return 1;
    }

  free (query_str);
  
  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error (_("MySQL: can't store result: %s"), mysql_error (m));
      mysql_close (m);
      return 1;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error (_("MySQL: can't fetch row: %s"), mysql_error (m));
      mysql_close (m);
      return 1;
    }

  rc = strcmp (row[0], crypt (pass, row[0]));
  mysql_free_result (res);
  mysql_close (m);
  return rc;
}

#endif
