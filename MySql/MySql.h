#include <config.h>

#ifdef HAVE_MYSQL
extern char *sql_getpwnam_query;
extern char *sql_getpass_query;
extern char *sql_getpwuid_query;
extern char *sql_host;
extern char *sql_user;
extern char *sql_passwd;
extern char *sql_db;
extern char *sql_socket; 
extern int  sql_port;

struct passwd *getMpwnam (const char *username);
struct passwd *getMpwuid (uid_t *puid);
struct spwd *getMspnam (const char *username);

#define MHOST NULL              /* Hostname to connect to. NULL for UNIX
                                   socket connection */
#define MPORT 0                 /* Port number to connect to. 0 means default
	                           MySQL port (3300) */
#define MSOCKET NULL            /* Socket name to use. Valid only if
				   connecting via UNIX sockets */
#define MFLAGS 0                /* Special user flags. It is safe to leave
				   this untouched */
#define MUSER "accounts"        /* Username for mysql access */
#define MPASS "yurpass"         /* Password for mysql access */
#define MDB "accounts"          /* Database Name */


#endif /* HAVE_MYSQL */
