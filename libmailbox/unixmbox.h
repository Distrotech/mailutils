/* copyright and license info go here */

#ifndef _UNIXMBOX_H
#define _UNIXMBOX_H	1
#include <mailbox.h>
#include "config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

typedef struct _unixmbox_message
  {
    off_t header;
    off_t body;
    off_t end;
    short deleted;
  }
unixmbox_message;

typedef struct _unixmbox_data
  {
    unixmbox_message *messages;
    FILE *file;
  }
unixmbox_data;

int unixmbox_open (mailbox *mbox);
int unixmbox_close (mailbox *mbox);
int unixmbox_delete (mailbox *mbox, int num);
int unixmbox_undelete (mailbox *mbox, int num);
int unixmbox_expunge (mailbox *mbox);
int unixmbox_is_deleted (mailbox *mbox, int num);
int unixmbox_add_message (mailbox *mbox, char *message);
char *unixmbox_get_body (mailbox *mbox, int num);
char *unixmbox_get_header (mailbox *mbox, int num);

#endif
