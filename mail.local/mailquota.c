/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <mail.local.h>

#if defined (USE_DBM) || defined (USE_SQL)

#define DEFRETVAL MQUOTA_UNLIMITED

size_t groupquota = 5*1024*1024UL;
static int get_size (char *, size_t *, char **);

int
get_size (char *str, size_t *size, char **endp)
{
  size_t s;

  s = strtol (str, &str, 0);
  switch (*str)
    {
    case 0:
      break;
      
    case 'k':
    case 'K':
      s *= 1024;
      break;
      
    case 'm':
    case 'M':
      s *= 1024*1024;
      break;
      
    default:
      *endp = str;
      return -1;
    }
  *size = s;
  return 0;
}

#define RETR_OK        0
#define RETR_UNLIMITED -1
#define RETR_FAILURE   1

int
fail_retrieve_quota (char *name, size_t *quota)
{
  mu_error (_("No quota retrieving mechanism"));
  return RETR_FAILURE;
}

#ifdef USE_DBM
int
dbm_retrieve_quota (char *name, size_t *quota)
{
  DBM_FILE db;
  DBM_DATUM named, contentd;
  char buffer[64];
  int unlimited = 0;
  int rc;

  if (!quotadbname || mu_dbm_open (quotadbname, &db, MU_STREAM_READ, 0600)) 
    return RETR_FAILURE;

  memset (&named, 0, sizeof named);
  memset (&contentd, 0, sizeof contentd);
  MU_DATUM_PTR (named) = name;
  MU_DATUM_SIZE (named) = strlen (name);
  rc = mu_dbm_fetch (db, named, &contentd);
  if (rc || !MU_DATUM_PTR (contentd))
    {
      /* User not in database, try default quota */
      memset (&named, 0, sizeof named);
      MU_DATUM_PTR (named) = "DEFAULT";
      MU_DATUM_SIZE (named) = strlen ("DEFAULT");
      rc = mu_dbm_fetch (db, named, &contentd);
      if (rc)
	{
	  /*mu_error (_("can't fetch data: %s"), strerror (rc));*/
	  return RETR_FAILURE;
	}
      if (!MU_DATUM_PTR (contentd))
	return RETR_FAILURE;
    }

  if (strncasecmp("none",
		  MU_DATUM_PTR (contentd),
		  MU_DATUM_SIZE (contentd)) == 0) 
      unlimited = 1;
  else if (MU_DATUM_SIZE (contentd) > sizeof(buffer)-1)
    {
      mu_error (_("mailbox quota for `%s' is too big: %d digits"),
		name, MU_DATUM_SIZE (contentd));
      *quota = groupquota;
    }
  else
    {
      char *p;
		
      strncpy(buffer, MU_DATUM_PTR (contentd), MU_DATUM_SIZE (contentd));
      buffer[MU_DATUM_SIZE (contentd)] = 0;
      *quota = strtoul (buffer, &p, 0);
      if (get_size (buffer, quota, &p))
	{
	  mu_error (_("bogus mailbox quota for `%s' (near `%s')"), name, p);
	  *quota = groupquota;
	}
    }

  mu_dbm_close (db);
  
  return unlimited ? RETR_UNLIMITED : RETR_OK;
}

# define default_retrieve_quota dbm_retrieve_quota
#else
# define default_retrieve_quota fail_retrieve_quota
#endif

#ifdef USE_SQL
#include <mailutils/sql.h>

/* FIXME: defined in auth/sql.c */
extern int sql_interface;
extern char *mu_sql_host;
extern char *mu_sql_user;
extern char *mu_sql_passwd;
extern char *mu_sql_db;
extern int  mu_sql_port;
extern char *mu_sql_expand_query (const char *query, const char *ustr);

int
sql_retrieve_quota (char *name, size_t *quota)
{
  mu_sql_connection_t conn;
  char *query_str;
  int rc, status;
  char *tmp;
  size_t n;
  
  query_str = mu_sql_expand_query (quota_query, name);
  if (!query_str)
    return RETR_FAILURE;

  status = mu_sql_connection_init (&conn,
				   sql_interface,
				   mu_sql_host,
				   mu_sql_port,
				   mu_sql_user,
				   mu_sql_passwd,
				   mu_sql_db);
  
  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return RETR_FAILURE;
    }

  status = mu_sql_connect (conn);

  if (status)
    {
      mu_error ("%s. SQL error: %s",
		mu_strerror (status), mu_sql_strerror (conn));
      mu_sql_connection_destroy (&conn);
      free (query_str);
      return  RETR_FAILURE;
    }
  
  status = mu_sql_query (conn, query_str);
  free (query_str);
  
  if (status)
    {
      mu_error (_("SQL Query failed: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return  RETR_FAILURE;
    }

  status = mu_sql_store_result (conn);

  if (status)
    {
      mu_error (_("cannot store SQL result: %s"),
		(status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
	 	                          mu_strerror (status));
      mu_sql_connection_destroy (&conn);
      return RETR_FAILURE;
    }

  mu_sql_num_tuples (conn, &n);
  if (n == 0)
    {
      rc = RETR_FAILURE;
    }
  else
    {
      rc = RETR_OK;
      tmp = NULL;
      status = mu_sql_get_column (conn, 0, 0, &tmp);
      if (status)
	{
	  mu_error (_("cannot retrieve SQL quota: %s"),
		    (status == MU_ERR_SQL) ?  mu_sql_strerror (conn) :
		    mu_strerror (status));
	  rc = RETR_FAILURE;
	}
      else if (tmp == NULL || strcasecmp (tmp, "none") == 0)
	rc = RETR_UNLIMITED;
      else
	{
	  char *p;
	  
	  if (get_size (tmp, quota, &p))
	    {
	      mu_error (_("bogus mailbox quota for `%s' (near `%s')"),
			name, p);
	      *quota = groupquota;
	    }
	}
    }
  
  mu_sql_release_result (conn);
  mu_sql_disconnect (conn);
  mu_sql_connection_destroy (&conn);
  return rc;
}
#endif


static int
retrieve_quota (char *name, size_t *quota)
{
#ifdef USE_SQL
  if (quota_query)
    return sql_retrieve_quota (name, quota);
#endif
  return default_retrieve_quota (name, quota);
}

int
check_quota (char *name, size_t size, size_t *rest)
{
  size_t quota;
  
  switch (retrieve_quota (name, &quota))
    {
    case RETR_FAILURE:
      return DEFRETVAL;
      
    case RETR_UNLIMITED:
      return MQUOTA_UNLIMITED;

    case RETR_OK:
      if (quota < size)  /* Mailbox full */
	return MQUOTA_EXCEEDED;
	
      if (rest)
	*rest = quota - size;
    }
  
  return MQUOTA_OK;
      
}
  
#endif /* USE_MAIL_QUOTA */
