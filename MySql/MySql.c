#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>

#include <config.h>

#ifdef HAVE_MYSQL

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif /* HAVE_SHADOW_H */

#include <mysql/mysql.h>
#include "MySql.h"

struct passwd *getMpwnam (const char *username)
{
	char QueryStr[1024];
	MYSQL *m;
	MYSQL_RES *res;
	MYSQL_ROW row;
	struct passwd *tpw;

	m = mysql_init(0);

        if (!m)
		return(NULL);

	if (!mysql_real_connect(m, NULL, MUSER, MPASS, MDB, 0, NULL, 0))
		return(NULL);

	memset((char *)QueryStr, '\0', 1024);

	sprintf(QueryStr, "select %s,%s,%s,%s,%s from %s where %s = '%s'", Mpassword, Muid, Mgid, Mhomedir, Mshell, Mtable, Musername, username);

	if (mysql_query(m, QueryStr) != 0)
		return(NULL);

	if ((res = mysql_store_result(m)) == NULL)
		return(NULL);

	if ((row = mysql_fetch_row(res)) == NULL)
		return(NULL);

	tpw = (struct passwd *)malloc(sizeof(struct passwd));
	
	tpw->pw_name = malloc(strlen(username)+1);
	strcpy(tpw->pw_name, username);

	tpw->pw_passwd = malloc(strlen(row[0])+1);
	strcpy(tpw->pw_passwd, row[0]);

	tpw->pw_uid = atoi(row[1]);
	tpw->pw_gid = atoi(row[2]);
	
	tpw->pw_gecos = malloc(strlen("Mysql User")+1);
	strcpy(tpw->pw_gecos, "Mysql User");

	tpw->pw_dir = malloc(strlen(row[3])+1);
	strcpy(tpw->pw_dir, row[3]);

	tpw->pw_shell = malloc(strlen(row[4])+1);
	strcpy(tpw->pw_shell, row[4]);
	
	mysql_free_result(res);	
	return(tpw);
}


#ifdef HAVE_SHADOW_H

struct spwd *getMspnam (const char *username)
{
	char QueryStr[1024];
	MYSQL *m;
	MYSQL_RES *res;
	MYSQL_ROW row;
	struct spwd *tpw;

	m = mysql_init(0);

        if (!m)
		return(NULL);

	if (!mysql_real_connect(m, NULL, MUSER, MPASS, MDB, 0, NULL, 0))
		return(NULL);

	memset((char *)QueryStr, '\0', 1024);
	sprintf(QueryStr, "select %s from %s where %s = '%s'", Mpassword, Mtable, Musername, username);

	if (mysql_query(m, QueryStr) != 0)
		return(NULL);
	 
	if ((res = mysql_store_result(m)) == NULL)
		return(NULL);

	if ((row = mysql_fetch_row(res)) == NULL)
		return(NULL);

	tpw = (struct spwd *)malloc(sizeof(struct spwd));
	
	tpw->sp_namp = malloc(strlen(username)+1);
	strcpy(tpw->sp_namp, username);

	tpw->sp_pwdp = malloc(strlen(row[0])+1);
	strcpy(tpw->sp_pwdp, row[0]);

	tpw->sp_lstchg = 11428;
	tpw->sp_min = 0;
	tpw->sp_max = 99999;
	tpw->sp_warn = 7;

	mysql_free_result(res);	
	return(tpw);
}

#endif /* HAVE_SHADOW_H */

#endif /* HAVE_MYSQL */
