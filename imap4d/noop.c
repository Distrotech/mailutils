#include "noop.h"

STATUS imap_noop(Command *command) {

	if( command->args )
		return BAD;

	fprintf(output, "* OK XSTATE %d\r\n", state);

	return OK;
}
