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

  if (!mysql_real_connect (m, NULL, MUSER, MPASS, MDB, 0, NULL, 0))
    return NULL;

  asprintf (&QueryStr,
	   "select %s,%s,%s,%s,%s from %s where %s = '%s'",
	   Mpassword, Muid, Mgid, Mhomedir, Mshell, Mtable,
	   Musername, username);

  if (!QueryStr)
    return NULL;
  
  if (mysql_query (m, QueryStr) != 0)
    return NULL;

  if ((res = mysql_store_result (m)) == NULL)
    return NULL;

  if ((row = mysql_fetch_row (res)) == NULL)
    return NULL;

  if (!tpw)
    tpw = (struct passwd *)xmalloc (sizeof (struct passwd));
  tpw->pw_name = xmalloc (strlen (username)+1);
  strcpy (tpw->pw_name, username);

  tpw->pw_passwd = xmalloc (strlen (row[0])+1);
  strcpy (tpw->pw_passwd, row[0]);

  tpw->pw_uid = atoi (row[1]);
  tpw->pw_gid = atoi (row[2]);
	
  tpw->pw_gecos = xmalloc (strlen ("Mysql User")+1);
  strcpy (tpw->pw_gecos, "Mysql User");

  tpw->pw_dir = xmalloc (strlen (row[3])+1);
  strcpy (tpw->pw_dir, row[3]);

  tpw->pw_shell = xmalloc (strlen (row[4])+1);
  strcpy (tpw->pw_shell, row[4]);
	
  mysql_free_result (res);	
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
  
  if (!mysql_real_connect (m, NULL, MUSER, MPASS, MDB, 0, NULL, 0))
    return NULL;

  asprintf (&QueryStr,
	   "select %s from %s where %s = '%s'",
	   Mpassword, Mtable, Musername, username);

  if (mysql_query (m, QueryStr) != 0)
    return NULL;
	 
  if ((res = mysql_store_result (m)) == NULL)
    return NULL;

  if ((row = mysql_fetch_row (res)) == NULL)
    return NULL;

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
  return tpw;
}

#endif /* HAVE_SHADOW_H */

#endif /* HAVE_MYSQL */
