/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* First draft by Jeff Bailey based on mbox by Alain Magloire */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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
#  define _XOPEN_SOURCE  500
#  include <pthread.h>
# endif
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailbox0.h>
#include <registrar0.h>

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
#include <mailutils/mutil.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>

/* Mailbox concrete implementation.  */
static int maildir_open                  __P ((mailbox_t, int));
static int maildir_close                 __P ((mailbox_t));
static void maildir_destroy               __P ((mailbox_t));
static int maildir_get_message           __P ((mailbox_t, size_t, message_t *));
/* static int maildir_get_message_by_uid    __P ((mailbox_t, size_t, message_t *)); */
static int maildir_append_message        __P ((mailbox_t, message_t));
static int maildir_messages_count        __P ((mailbox_t, size_t *));
static int maildir_messages_recent       __P ((mailbox_t, size_t *));
static int maildir_message_unseen        __P ((mailbox_t, size_t *));
static int maildir_expunge               __P ((mailbox_t));
static int maildir_save_attributes       __P ((mailbox_t));
static int maildir_uidvalidity           __P ((mailbox_t, unsigned long *));
static int maildir_uidnext               __P ((mailbox_t, size_t *));
static int maildir_scan                  __P ((mailbox_t, size_t, size_t *));
static int maildir_is_updated            __P ((mailbox_t));
static int maildir_get_size              __P ((mailbox_t, off_t *));

int
_mailbox_maildir_init (mailbox_t mailbox)
{

  if (mailbox == NULL)
    return EINVAL;

  /* Overloading the defaults.  */
  mailbox->_destroy = maildir_destroy;

  mailbox->_open = maildir_open;
  mailbox->_close = maildir_close;

  /* Overloading of the entire mailbox object methods.  */
  mailbox->_get_message = maildir_get_message;
  mailbox->_append_message = maildir_append_message;
  mailbox->_messages_count = maildir_messages_count;
  mailbox->_messages_recent = maildir_messages_recent;
  mailbox->_message_unseen = maildir_message_unseen;
  mailbox->_expunge = maildir_expunge;
  mailbox->_save_attributes = maildir_save_attributes;
  mailbox->_uidvalidity = maildir_uidvalidity;
  mailbox->_uidnext = maildir_uidnext;

  mailbox->_scan = maildir_scan;
  mailbox->_is_updated = maildir_is_updated;

  mailbox->_get_size = maildir_get_size;

  return 0; /* okdoke */
}

/* Destruct maildir setup */
static void
maildir_destroy (mailbox_t mailbox)
{
  return;
}

/* Open the file.  For MU_STREAM_READ, the code tries mmap() first and fall
   back to normal file.  */
static int
maildir_open (mailbox_t mailbox, int flags)
{
  return -1;
}

static int
maildir_close (mailbox_t mailbox)
{
  return -1;
}

/* Cover function that call the real thing, maildir_scan(), with
   notification set.  */
static int
maildir_scan (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  return 0;
}

/* FIXME:  How to handle a shrink ? meaning, the &^$^@%#@^& user start two
   browsers and deleted emails in one session.  My views is that we should
   scream bloody murder and hunt them with a machette. But for now just play
   dumb, but maybe the best approach is to pack our things and leave
   .i.e exit()/abort().  */
static int
maildir_is_updated (mailbox_t mailbox)
{
  return -1;
}

static int
maildir_expunge (mailbox_t mailbox)
{
  return -1;
}

static int
maildir_get_size (mailbox_t mailbox, off_t *psize)
{
  return -1;
}

static int
maildir_save_attributes (mailbox_t mailbox)
{
  return -1;
}

static int
maildir_get_message (mailbox_t mailbox, size_t msgno, message_t *pmsg)
{
  return -1;
}

static int
maildir_append_message (mailbox_t mailbox, message_t msg)
{
  return -1;
}

static int
maildir_messages_count (mailbox_t mailbox, size_t *pcount)
{
  return -1;
}

/* A "recent" message is the one not marked with MU_ATTRIBUTE_SEEN
   ('O' in the Status header), i.e. a message that is first seen
   by the current session (see attributes.h) */
static int
maildir_messages_recent (mailbox_t mailbox, size_t *pcount)
{
  return -1;
}

/* An "unseen" message is the one that has not been read yet */
static int
maildir_message_unseen (mailbox_t mailbox, size_t *pmsgno)
{
  return -1;
}

static int
maildir_uidvalidity (mailbox_t mailbox, unsigned long *puidvalidity)
{
  return -1;
}

static int
maildir_uidnext (mailbox_t mailbox, size_t *puidnext)
{
  return -1;
}

