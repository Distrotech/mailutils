#include "capability.h"

STATUS imap_capability(Command *command) {

	if( command->args )
		return BAD;

	fprintf(output, "* CAPABILITY IMAP4rev1\r\n");

	return OK;
}
