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

#ifndef _MH_H
#define _MH_H	1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <mailbox.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* FIXME need auto* wrapper */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <time.h>

#include <fcntl.h>

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

typedef struct _mh_message
  {
    off_t body;
    char deleted;
  }
mh_message;

typedef struct _mh_data
  {
    mh_message *messages;
    DIR *dir;
    unsigned int sequence;
  }
mh_data;

int mh_open (mailbox *mbox);
int mh_close (mailbox *mbox);
int mh_delete (mailbox *mbox, unsigned int num);
int mh_undelete (mailbox *mbox, unsigned int num);
int mh_expunge (mailbox *mbox);
int mh_scan (mailbox *mbox);
int mh_is_deleted (mailbox *mbox, unsigned int num);
int mh_is_updated (mailbox *mbox);
int mh_lock (mailbox *mbox, mailbox_lock_t mode);
int mh_add_message (mailbox *mbox, char *message);
char *mh_get_body (mailbox *mbox, unsigned int num);
char *mh_get_header (mailbox *mbox, unsigned int num);

#endif
