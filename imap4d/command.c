#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"

Command* parse_command(char *string) {

	size_t len, tmp;
	Command *command;

	if( string == NULL )
		return NULL;

	command = malloc(sizeof(Command));
	memset(command, '\0', sizeof(Command));

	len = strcspn(string, " ");
	if( len > 0 ) {
		tmp = strcspn(string, "\r\n");
		string[tmp] = '\0';
		strncpy(command->tag, string, len > 15 ? 15 : len);
	} else {
		strcpy(command->tag, "gibberish");
		return command;
	}

	tmp = len;
	++tmp; /* skip space char */
	len = strcspn(string + tmp, " ");
	if( len > 0 )
		strncpy(command->cmd, string + tmp, len > 15 ? 15 : len);
	else {
		strcpy(command->cmd, "gibberish");
		return command;
	}	
	/* args is the rest of the string */
	command->args = string + len + tmp + 1;

	if( command->args[0] == '\0' ) /* no args */
		command->args = NULL;

	return command;
}

void free_command(Command *command) {

	free(command);
}
