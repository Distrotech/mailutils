#include "logout.h"

STATUS imap_logout(Command *command) {

	if( command->args )
		return BAD;

	/* finish everything */

	fprintf(output, "* BYE IMAP4rev1 terminating connection\r\n");
#ifdef STATISTICS
	statistics();
#endif

	state = LOGOUT;

	return OK;
}
