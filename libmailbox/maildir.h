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

#ifndef _MAILDIR_H
#define _MAILDIR_H	1
#include <mailbox.h>
#include "config.h"

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

typedef struct _maildir_message
  {
    off_t body;
    char deleted;
    enum { new, tmp, cur } location;
    char *file;
    char *info;
  }
maildir_message;

typedef struct _maildir_data
  {
    maildir_message *messages;
    DIR *new;
    DIR *cur;
    DIR *tmp;
    time_t time;
    pid_t pid;
    unsigned int sequence;
    char *host;
    time_t last_mod_time;
  }
maildir_data;

int maildir_open (mailbox *mbox);
int maildir_close (mailbox *mbox);
int maildir_delete (mailbox *mbox, unsigned int num);
int maildir_undelete (mailbox *mbox, unsigned int num);
int maildir_expunge (mailbox *mbox);
int maildir_scan (mailbox *mbox);
int maildir_is_deleted (mailbox *mbox, unsigned int num);
int maildir_is_updated (mailbox *mbox);
int maildir_lock (mailbox *mbox, mailbox_lock_t mode);
int maildir_add_message (mailbox *mbox, char *message);
char *maildir_get_body (mailbox *mbox, unsigned int num);
char *maildir_get_header (mailbox *mbox, unsigned int num);

#endif
