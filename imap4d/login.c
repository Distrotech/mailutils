#define _XOPEN_SOURCE
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>
#include <shadow.h>
#include <string.h>
#include <sys/types.h>

#include "login.h"

STATUS imap_login(Command *command) {

	char user[256];
	char *pass;
	struct passwd *pw;
	struct spwd *shadow;
	size_t len;

	if( state != NON_AUTH )
		return BAD;
	if( command->args == NULL )
		return BAD;

	len = strcspn(command->args, " ");
	if( len == 0 || len > 255 )
		return BAD;
	memset(user, '\0', 256);
	strncpy(user, command->args, len);
	pass = command->args + len;
	++pass; /* skip the space */
	if( isspace(pass[0]) )
		return BAD;
	pw = getpwnam(user);
	if ( pw == NULL ) {
		syslog(LOG_WARNING, "invalid user: %s\n", user);
		return NO;
	}
	if( strcmp(pw->pw_passwd, crypt(pass, pw->pw_passwd)) ) {
		shadow = getspnam(user);
		if( shadow == NULL ) {
			syslog(LOG_WARNING, "invalid user: %s\n", user);
			return NO;
		}
		if( strcmp(shadow->sp_pwdp, crypt(pass, shadow->sp_pwdp)) ) {
			syslog(LOG_WARNING, "invalid user: %s\n", user);
			return NO;
		}
	}
	memset(pass, '\0', sizeof(pass)); /* paranoia */
	state = AUTH;
	syslog(LOG_INFO, "user: %s, logged in\n", user);
	return OK;
}
