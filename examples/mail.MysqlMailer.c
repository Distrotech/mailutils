#include <stdlib.h>
#include <stdio.h>
#include <mysql/mysql.h>
#include <unistd.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>

/**********************************************************************
** mailer to go with gnu-pop3d patch, works with sendmail
** add the following in sendmail.cf right after virtusertable ruleset
** R$* < $* @ yourdomain . net . >        $#MysqlMailer $: $1
**
** Then after local mailer, add the following
** MMysqlMailer,   P=/usr/local/bin/mail.MysqlMailer, F=lsDFMoqeu9, S=10/30, R=20/40,
**		A=mail.MysqlMailer $u
**
** compile with gcc -o mail.MysqlMailer mail.MysqlMailer.c -lmysqlclient
**
** chown it so it is owned by same owner of your psuedo mails (mine is monly)
** and same group as owner sendmail runs as
** then chmod 4711. If you have local users on your box (shell accounts) you may want
** to declare AGENT as read-only in /etc/profile (declare -r AGENT).
**
**		Author: Jim Hull (08-24-2001)
**		imaginos@imaginos.net
**********************************************************************/

#define USERNAME "username" 	/* username field */
#define TABLE "table" 		/* table name */
#define Muser "user"		/* Mysql username */
#define Mpass "password"	/* Mysql password */
#define Mdb "db"		/* Mysql Database Name */

int main(int argc, char **argv)
{
	FILE *f;
	char QueryStr[1024], *user, path[128], output[1024], *agent;
	MYSQL *m;
	MYSQL_RES *res;
	MYSQL_ROW row;
	int i;

	if (argc != 2)
		exit(EX_NOUSER);

	agent = getenv("AGENT");

	if (!agent)
		exit(EX_NOUSER);

	if (strcmp(getenv("AGENT"), "sendmail") != 0)
		exit(EX_NOUSER);
		
	user = strdup(argv[1]);

	memset((char *)QueryStr, '\0', 1024);
	memset((char *)path, '\0', 128);

	m = mysql_init(0);

        if (!m)
		exit(EX_NOUSER);

	if (!mysql_real_connect(m, NULL, Muser, Mpass, Mdb, 0, NULL, 0))
		exit(EX_NOUSER);

	sprintf(QueryStr, "select %s from %s where %s = '%s' limit 1", USERNAME, TABLE, USERNAME, user);
	
	if (mysql_query(m, QueryStr) != 0)
		exit(EX_NOUSER);

	if ((res = mysql_store_result(m)) == NULL)
		exit(EX_NOUSER);

	if ((row = mysql_fetch_row(res)) == NULL)
		exit(EX_NOUSER);

	sprintf(path, "/var/spool/mail/%s", row[0]);

	f = fopen(path, "a");

	if (!f)
		exit(EX_NOUSER);

	while (!feof(stdin))
	{
		memset((char *)output, '\0', 1024);
		fgets(output, 1024, stdin);
		fprintf(f, "%s", output);
	}
	chmod(path, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP);
	fclose(f);
	exit(0);
}



