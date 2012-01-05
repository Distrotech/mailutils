/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#ifdef WITH_PTHREAD
# ifdef HAVE_PTHREAD_H
#  ifndef _XOPEN_SOURCE
#   define _XOPEN_SOURCE  500
#  endif
#  include <pthread.h>
# endif
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/sys/mailbox.h>
#include <mailutils/sys/registrar.h>

#include <mailutils/address.h>
#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/envelope.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/header.h>
#include <mailutils/locker.h>
#include <mailutils/message.h>
#include <mailutils/util.h>
#include <mailutils/nls.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>

struct _mbox_message;
struct _mbox_data;

typedef struct _mbox_data *mbox_data_t;
typedef struct _mbox_message *mbox_message_t;

struct _mbox_message
{
  /* Offset of the messages in the mailbox.  */
  mu_off_t envel_from;         /* Start of envelope (^From ) */
  mu_off_t envel_from_end;     /* End of envelope (terminating \n) */
  mu_off_t body;               /* Start of body */
  mu_off_t body_end;           /* End of body */

  size_t header_lines;         /* Number of lines in header */
  size_t body_lines;           /* Number of lines in message body */
  size_t uid;                  /* IMAP-style uid.  */
  int attr_flags;              /* Packed "Status:" attribute flags */

  mu_message_t message;        /* A message attached to it.  */
  mbox_data_t mud;             /* Reference to the containing UNIX mailbox */
};

/* The umessages is an array of pointers that contains umessages_count of
   mbox_message_t*; umessages[umessages_count].  We do it this way because
   realloc() can move everything to a new memory region and invalidate all
   the pointers.  Thanks to <Dave Inglis> for pointing this out.  The
   messages_count is the count number of messages parsed so far.  */
struct _mbox_data
{
  mbox_message_t *umessages;   /* Array.  */
  size_t umessages_count;      /* Number of slots in umessages. */
  size_t messages_count;       /* Number of used slots in umessages. */
  mu_off_t size;               /* Size of the mailbox.  */
  unsigned long uidvalidity;
  size_t uidnext;              /* Expected next UID value */
  char *name;                  /* Disk file name */

  mu_mailbox_t mailbox; /* Back pointer. */
};

int mbox_scan0 (mu_mailbox_t mailbox, size_t msgno, size_t *pcount,
		int do_notif);
int mbox_scan1 (mu_mailbox_t mailbox, mu_off_t offset, int do_notif);

#ifdef WITH_PTHREAD
void mbox_cleanup (void *arg);
#endif

