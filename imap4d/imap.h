#ifndef _MY_IMAP
#define _MY_IMAP

#include <stdio.h>
#include <syslog.h>

#include "imap_types.h"

FILE *input;
FILE *output;

/* current server state */
STATES state;

#endif
