#ifndef _MY_IMAP_COMMAND
#define _MY_IMAP_COMMAND

#include "imap_types.h"

Command *parse_command(char *string);
void free_command(Command *command);

#endif
