#include <config.h>

#ifdef HAVE_MYSQL

#define MHOST NULL              /* Hostname to connect to. NULL for UNIX
				   socket connection */ 
#define MPORT 0                 /* Port number to connect to. 0 means default
				   MySQL port (3300) */
#define MSOCKET NULL            /* Socket name to use. Valid only if connecting
				   via UNIX sockets */
#define MFLAGS 0                /* Special user flags. It is safe to leave
				   this untouched */
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
