#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "imap.h"
#include "imap_commands.h"
#include "command.h"
#include "readline.h"

static void init(void);
static int mainloop(void);
static void cleanup(void);
static void greeting(void);
static STATUS action(Command *command);

/* state the server is in */
STATES state = NON_AUTH;

FILE *input = NULL;
FILE *output = NULL;

/* output strings, maps STATUS to strings */
static char *status_word[3] = {"BAD", "NO", "OK"};
static char *status_code[3] = {"invalid", "failed", "completed"};


int main(int argc, char *argv[]) {

	input = stdin;
	output = stdout;

	init();

	greeting();

	mainloop();

	cleanup();
	
	fclose(input);
	fclose(output);

	return 0;
}

static void init(void) {

	struct group *gr;

	gr = getgrnam("mail");
	if( gr == NULL )
		exit(-1);
	if( setgid(gr->gr_gid) == -1 )
		exit(-1);
	openlog("GNU IMAP4", LOG_PID, LOG_MAIL);
	if( output == stdout )
		setvbuf(output, NULL, _IOLBF, 0);
}

static void cleanup(void) {
	closelog();
}

static void greeting(void) {

	fprintf(output, "* OK IMAP4rev1 Service Ready\r\n");
}

static int mainloop(void) {

	char *client_string;
	Command *command;
	STATUS status;

	while( (client_string = read_a_line(input)) != NULL ) {
		command = parse_command(client_string);
		status = action(command);

		fprintf(output, "%s %s %s %s\r\n", command->tag, status_word[status],
				command->cmd, status_code[status]);

		free_command(command);
		free(client_string);

		if( state == LOGOUT ) /* all done, let's go */
			break;
	}

	return 1;
}

STATUS action(Command *command) {

	if( strcasecmp(command->cmd, "capability") == 0 ) {
		return imap_capability(command);
	}
	else if( strcasecmp(command->cmd, "noop") == 0 ) {
		return imap_noop(command);
	}
	else if( strcasecmp(command->cmd, "logout") == 0 ) {
		return imap_logout(command);
	}
	else if( strcasecmp(command->cmd, "login") == 0 ) {
		return imap_login(command);
	}

	return BAD;
}
