/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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
    int lockmode;
  }
unixmbox_data;

int unixmbox_open (mailbox *mbox);
int unixmbox_close (mailbox *mbox);
int unixmbox_delete (mailbox *mbox, unsigned int num);
int unixmbox_undelete (mailbox *mbox, unsigned int num);
int unixmbox_expunge (mailbox *mbox);
int unixmbox_is_deleted (mailbox *mbox, unsigned int num);
int unixmbox_lock (mailbox *mbox, int mode);
int unixmbox_add_message (mailbox *mbox, char *message);
char *unixmbox_get_body (mailbox *mbox, unsigned int num);
char *unixmbox_get_header (mailbox *mbox, unsigned int num);

#endif
