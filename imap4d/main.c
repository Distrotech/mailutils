#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "imap.h"
#include "imap_commands.h"
#include "command.h"

static void init(void);
static int mainloop(void);
static void cleanup(void);
static void greeting(void);
static Command* parse_command(char *string);

#define action(cmd) cmd->info->action ? cmd->info->action(cmd) : BAD;

#define free_command(cmd) free(cmd);

/* state the server is in */
STATES state = NON_AUTH;

FILE *input = NULL;
FILE *output = NULL;

/* output strings, maps STATUS to strings */
static char *status_word[3] = {"BAD", "NO", "OK"};
static char *status_code[3] = {"invalid", "failed", "completed"};

static imap4_cmd_t COMMANDS[] = {
	{ "capability", imap_capability, , , },
	{ "noop"      , imap_noop,       , , },
	{ "logout"    , imap_logout,     , , },
	{ "login"     , imap_login,      , , },
	{ "",         , NULL,            , , }
};


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

	char *client_string = NULL;
	size_t len = 0;
	Command *command;
	STATUS status;

	while( getline(&client_string, &len, input) != -1 ) {
		command = parse_command(client_string);
		if(command) {
			status = action(command);
			fprintf(output, "%s %s %s %s\r\n", command->tag, status_word[status],
					command->cmd, status_code[status]);
			free_command(command);
		}
		else {
			/* FIXME: properly handle this */
			fprintf(output, "");
		}

		if( state == LOGOUT ) /* all done, let's go */
			break;
	}
	free(client_string);

	return 1;
}

imap4_action_t check_command(const char *command) {

	for(i = 0; COMMANDS[i]; ++i) {
		if(strcasecmp(command, COMMANDS[i].cmd) == 0)
			return COMMANDS[i];
	}
	return NULL;
}

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

	command->info = check_command(command->cmd);

	return command;
}
