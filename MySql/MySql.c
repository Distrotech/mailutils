#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>


#ifdef HAVE_MYSQL

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif /* HAVE_SHADOW_H */

#include <mysql/mysql.h>
#include "MySql.h"

extern void *xmalloc (size_t);

static char *
sql_expand_query (const char *query, const char *ustr)
{
  char *p, *q, *res;
  int len;

  if (!query)
    return NULL;
  
  /* Compute resulting query length */
  for (len = 0, p = query; *p; )
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

  for (p = query, q = res; *p; )
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

struct passwd *
getMpwnam (const char *username)
{
  char *QueryStr = NULL;
  MYSQL *m;
  MYSQL_RES *res;
  MYSQL_ROW row;
  static struct passwd *tpw;

  if (tpw)
    {
      free (tpw->pw_name);
      free (tpw->pw_passwd);
      free (tpw->pw_gecos);
      free (tpw->pw_dir);
      free (tpw->pw_shell);
    }
      
  m = mysql_init (0);

  if (!m)
    return NULL;

  if (!mysql_real_connect (m, sql_host, sql_user, sql_passwd, sql_db, sql_port,
			   sql_socket, MFLAGS))
    {
      mu_error ("MySQL: connect failed: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }

  QueryStr = sql_expand_query (sql_getpwnam_query, username);

  if (!QueryStr)
    {
      mysql_close (m);
      return NULL;
    }
  
  if (mysql_query (m, QueryStr) != 0)
    {
      mu_error ("MySQL: query failed: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }

  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error ("MySQL: can't store result: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error ("MySQL: can't fetch row: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }
  
  if (!tpw)
    tpw = (struct passwd *)xmalloc (sizeof (struct passwd));
  tpw->pw_name = xmalloc (strlen (row[0])+1);
  strcpy (tpw->pw_name, row[0]);

  tpw->pw_passwd = xmalloc (strlen (row[1])+1);
  strcpy (tpw->pw_passwd, row[1]);

  tpw->pw_uid = atoi (row[2]);
  tpw->pw_gid = atoi (row[3]);
	
  tpw->pw_gecos = xmalloc (strlen ("Mysql User")+1);
  strcpy (tpw->pw_gecos, "Mysql User");

  tpw->pw_dir = xmalloc (strlen (row[4])+1);
  strcpy (tpw->pw_dir, row[4]);

  tpw->pw_shell = xmalloc (strlen (row[5])+1);
  strcpy (tpw->pw_shell, row[5]);
	
  mysql_free_result (res);
  mysql_close (m);
  return tpw;
}

struct passwd *
getMpwuid (uid_t uid)
{
  char *QueryStr = NULL;
  MYSQL *m;
  MYSQL_RES *res;
  MYSQL_ROW row;
  static struct passwd *tpw;
  char uidstr[64];
  
  if (tpw)
    {
      free (tpw->pw_name);
      free (tpw->pw_passwd);
      free (tpw->pw_gecos);
      free (tpw->pw_dir);
      free (tpw->pw_shell);
    }
      
  m = mysql_init (0);

  if (!m)
    return NULL;

  if (!mysql_real_connect (m, sql_host, sql_user, sql_passwd, sql_db, sql_port,
			   sql_socket, MFLAGS))
    {
      mu_error ("MySQL: connect failed: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }

  snprintf (uidstr, sizeof (uidstr), "%u", uid);
  QueryStr = sql_expand_query (sql_getpwuid_query, uidstr);

  if (!QueryStr)
    {
      mysql_close (m);
      return NULL;
    }
  
  if (mysql_query (m, QueryStr) != 0)
    {
      mu_error ("MySQL: query failed: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }

  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error ("MySQL: can't store result: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error ("MySQL: can't fetch row: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }
  
  if (!tpw)
    tpw = (struct passwd *)xmalloc (sizeof (struct passwd));
  tpw->pw_name = xmalloc (strlen (row[0]+1));
  strcpy (tpw->pw_name, row[0]);

  tpw->pw_passwd = xmalloc (strlen (row[1])+1);
  strcpy (tpw->pw_passwd, row[0]);

  tpw->pw_uid = atoi (row[2]);
  tpw->pw_gid = atoi (row[3]);
	
  tpw->pw_gecos = xmalloc (strlen ("Mysql User")+1);
  strcpy (tpw->pw_gecos, "Mysql User");

  tpw->pw_dir = xmalloc (strlen (row[4])+1);
  strcpy (tpw->pw_dir, row[4]);

  tpw->pw_shell = xmalloc (strlen (row[5])+1);
  strcpy (tpw->pw_shell, row[5]);
	
  mysql_free_result (res);
  mysql_close (m);
  return tpw;
}

#ifdef HAVE_SHADOW_H

struct spwd *
getMspnam (const char *username)
{
  char *QueryStr = NULL;
  MYSQL *m;
  MYSQL_RES *res;
  MYSQL_ROW row;
  static struct spwd *tpw;

  if (tpw)
    {
      free (tpw->sp_namp);
      free (tpw->sp_pwdp);
    }
      
  m = mysql_init (0);

  if (!m)
    return NULL;
  
  if (!mysql_real_connect (m, sql_host, sql_user, sql_passwd, sql_db, sql_port,
			   sql_socket, MFLAGS))
    {
      mu_error ("MySQL: connect failed: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }

  QueryStr = sql_expand_query (sql_getpass_query, username);

  if (!QueryStr)
    {
      mysql_close (m);
      return NULL;
    }

  if (mysql_query (m, QueryStr) != 0)
    {
      mu_error ("MySQL: query failed: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }

  if ((res = mysql_store_result (m)) == NULL)
    {
      mu_error ("MySQL: can't store result: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }
  
  if ((row = mysql_fetch_row (res)) == NULL)
    {
      mu_error ("MySQL: can't fetch row: %s", mysql_error (m));
      mysql_close (m);
      return NULL;
    }

  if (!tpw)
    tpw = (struct spwd *)xmalloc (sizeof (struct spwd));
	
  tpw->sp_namp = xmalloc (strlen (username)+1);
  strcpy (tpw->sp_namp, username);

  tpw->sp_pwdp = xmalloc (strlen (row[0])+1);
  strcpy (tpw->sp_pwdp, row[0]);

  tpw->sp_lstchg = 11428;
  tpw->sp_min = 0;
  tpw->sp_max = 99999;
  tpw->sp_warn = 7;

  mysql_free_result (res);
  mysql_close (m);
  return tpw;
}

#endif /* HAVE_SHADOW_H */

#endif /* HAVE_MYSQL */
