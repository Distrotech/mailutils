#include <config.h>

#ifdef HAVE_MYSQL

#define MUSER "accounts"	/* Username for mysql access */
#define MPASS "yurpass"		/* Password for mysql access */
#define MDB "accounts"		/* Database Name */
#define Mtable "users"		/* Table Name */
#define Musername "username"	/* username field */
#define Muid "uid"		/* uid field */
#define Mgid "gid"		/* gid field */
#define Mpassword "password"	/* password field */
#define Mhomedir "homedir"	/* homedir field */
#define Mshell "shell"		/* shell field */
#define Mcomment "comment"	/* comment field */

struct passwd *getMpwnam (const char *username);
struct spwd *getMspnam (const char *username);


#endif /* HAVE_MYSQL */
