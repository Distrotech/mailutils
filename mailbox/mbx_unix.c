/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* First draft by Alain Magloire */

#include <mailbox0.h>
#include <registrar0.h>
#include <message0.h>
#include <url0.h>
#include <io0.h>
#include <body0.h>
#include <attribute0.h>
#include <header.h>
#include <auth.h>
#include <locker.h>

//#define HAVE_PTHREAD_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <mailutils_errno.h>
#include <time.h>
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#else
# define flockfile(arg)
# define funlockfile(arg)
#endif
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define ATTRIBUTE_IS_DELETED(flag)        (flag & MU_ATTRIBUTE_DELETED)
#define ATTRIBUTE_IS_EQUAL(flag1, flag2)  (flag1 == flag2)

static int unix_create (mailbox_t *pmbox, const char *name);
static void unix_destroy (mailbox_t *pmbox);

struct mailbox_registrar _mailbox_unix_registrar =
{
  "UNIX MBOX",
  unix_create, unix_destroy
};

/* Keep the position of where the header and body starts and ends.
   old_flags is the "Status:" message.  */
typedef struct _unix_message
{
  /* Offset of the parts of the messages in the mailbox.  */
  off_t header_from;
  off_t header_from_end;
  /* Little hack to make things easier * when updating the attribute.  */
  off_t header_status;
  off_t header_status_end;
  off_t body;
  off_t body_end;

  /* The old_flags contains the definition of Header.  */
  int old_flags;
  /* The new_flags holds the attributes changes for the current session. We
     use this so when expunging we can tell when an attribute been modified.
     This is a big help so we can jump to the first modify email for speed
     in expunging (see mark dirty).  */
  int new_flags;

  size_t header_lines;
  size_t body_lines;
  stream_t stream;

  /* A message attach to it.  */
  message_t message;

} *unix_message_t;

/* The umessages is an array of pointers that contains umessages_count of
   unix_message_t*; umessages[umessages_count].  We do it this because
   realloc() can move everything to a new memory region and invalidating all
   the pointers someone has on the messages.  Thanks to <Dave Inglis> for
   pointing this out.  The messages_count is the count number of messages
   parsed so far.  */
typedef struct _unix_data
{
  unix_message_t *umessages;
  size_t umessages_count;
  size_t messages_count;
  stream_t stream;
  char *dirname;
  char *basename;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  off_t size;

} *unix_data_t;

static int unix_open (mailbox_t mbox, int flag);
static int unix_close (mailbox_t mbox);
static int unix_get_message (mailbox_t, size_t msgno, message_t *msg);
static int unix_append_message (mailbox_t, message_t msg);
static int unix_messages_count (mailbox_t, size_t *msgno);
static int unix_expunge (mailbox_t);
static int unix_num_deleted (mailbox_t, size_t *);
static int unix_scan (mailbox_t, size_t count, size_t *pcount);
static int unix_is_updated (mailbox_t);
static int unix_size (mailbox_t, off_t *size);


/* private stuff */
static int unix_scan0 (mailbox_t, size_t count, size_t *pcount, int);
static int unix_get_header_read (stream_t is, char *buffer, size_t len,
				 off_t off, size_t *pnread);
static int unix_get_fd (stream_t is, int *pfd);
static int unix_get_flags (attribute_t, int *);
static int unix_set_flags (attribute_t, int);
static int unix_unset_flags (attribute_t, int);
static int unix_readstream (stream_t is, char *buffer, size_t buflen,
				    off_t off, size_t *pnread);
static int unix_body_size (body_t body, size_t *psize);
static int unix_body_lines (body_t body, size_t *plines);
static int unix_msg_from (message_t msg, char *buf, size_t len,
				  size_t *pnwrite);
static int unix_msg_received (message_t msg, char *buf, size_t len,
				      size_t *pnwrite);
static int unix_lock (mailbox_t mbox, int flag);
static int unix_touchlock (mailbox_t mbox);
static int unix_unlock (mailbox_t mbox);
static int unix_ilock (mailbox_t mbox, int flag);
static int unix_iunlock (mailbox_t mbox);

/* We allocate the mailbox_t struct, but don't do any parsing on the name or
   even test for existence.  However we do strip any leading "unix:" part of
   the name, this is suppose to be the protocol/scheme name. Hopefully there
   will not be a mailbox name "unix:".  */
static int
unix_create (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;
  unix_data_t mud;
  const char *sep;
  size_t name_len;

  /* Sanity check.  */
  if (name == NULL || *name == '\0')
    return MU_ERROR_INVALID_ARG;

  name_len = strlen (name);

#define UNIX_SCHEME "unix:"
#define UNIX_SCHEME_LEN 5
#define SEPARATOR '/'

  /* Skip the url scheme.  */
  if (name_len > UNIX_SCHEME_LEN &&
      (name[0] == 'u' || name[0] == 'U') &&
      (name[1] == 'n' || name[1] == 'N') &&
      (name[2] == 'i' || name[2] == 'i') &&
      (name[3] == 'x' || name[3] == 'x' ) &&
      name[4] == ':')
    {
      name += UNIX_SCHEME_LEN;
      name_len -= UNIX_SCHEME_LEN;
    }

  /* Allocate memory for mbox.  */
  mbox = calloc (1, sizeof (*mbox));
  if (mbox == NULL)
    return MU_ERROR_OUT_OF_MEMORY;

  /* Allocate specific unix mbox data.  */
  mud = mbox->data = calloc (1, sizeof (*mud));
  if (mbox->data == NULL)
    {
      unix_destroy (&mbox);
      return MU_ERROR_OUT_OF_MEMORY;
    }

  /* Copy the name.  */
  mbox->name = calloc (name_len + 1, sizeof (char));
  if (mbox->name == NULL)
    {
      unix_destroy (&mbox);
      return MU_ERROR_OUT_OF_MEMORY;
    }
  memcpy (mbox->name, name, name_len);

  /* Save the basename and dirname.  We split the name, but this should
     probably be supported via "maildir:" or the mailbox mgr.  */
  sep = strrchr (name, SEPARATOR);
  if (sep)
    {
      /* Split it into two.  */
      mud->dirname = calloc ((sep - name) + 1, sizeof (char));
      if (mud->dirname == NULL)
	{
	  unix_destroy (&mbox);
	  return MU_ERROR_OUT_OF_MEMORY;
	}
      memcpy (mud->dirname, name, sep - name);

      ++sep;
      mud->basename = calloc (name_len - (sep - name) + 1, sizeof (char));
      if (mud->basename == NULL)
	{
	  unix_destroy (&mbox);
	  return MU_ERROR_OUT_OF_MEMORY;
	}
      memcpy (mud->basename, sep, name_len - (sep - name));
    }
  else
    {
      /* Use the relative directory ".".  */
      /* FIXME: should we call getcwd () instead ?  */
      mud->dirname = calloc (2 , sizeof (char));
      if (mud->dirname == NULL)
	{
	  unix_destroy (&mbox);
	  return MU_ERROR_OUT_OF_MEMORY;
	}
      mud->dirname[0] = '.';

      mud->basename = calloc (name_len + 1, sizeof (char));
      if (mud->basename == NULL)
	{
	  unix_destroy (&mbox);
	  return MU_ERROR_OUT_OF_MEMORY;
	}
      memcpy (mud->basename, name, name_len);
    }

#ifdef HAVE_PHTREAD_H
  /* Mutex when accessing the structure fields.  */
  /* FIXME: should we use rdwr locks instead ??  */
  pthread_mutex_init (&(mbox->mutex), NULL);
#endif

  mbox->_create = unix_create;
  mbox->_destroy = unix_destroy;

  mbox->_open = unix_open;
  mbox->_close = unix_close;

  /* Messages.  */
  mbox->_get_message = unix_get_message;
  mbox->_append_message = unix_append_message;
  mbox->_messages_count = unix_messages_count;
  mbox->_expunge = unix_expunge;
  mbox->_num_deleted = unix_num_deleted;

  mbox->_scan = unix_scan;
  mbox->_is_updated = unix_is_updated;

  mbox->_size = unix_size;

  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE, "unix_create (%s/%s)\n",
		 mud->dirname, mud->basename);
  (*pmbox) = mbox;

  return 0; /* okdoke */
}

static void
unix_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mailbox_t mbox = *pmbox;
      unix_close (mbox);
      if (mbox->data)
	{
	  size_t i;
	  unix_data_t mud = mbox->data;
	  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE,
			 "unix_destroy (%s/%s)\n",
			 mud->dirname, mud->basename);
	  free (mud->dirname);
	  free (mud->basename);
	  for (i = 0; i < mud->umessages_count; i++)
	    {
	      unix_message_t mum = mud->umessages[i];
	      if (mum == NULL)
		{
		  message_destroy (&(mum->message), mum);
		  free (mum);
		}
	    }
	  free (mud->umessages);
	  free (mbox->data);
	}
      free (mbox->name);
      /* Free the event array.  */
      free (mbox->event);
      /* Destroy the url.  */
      if (mbox->url)
	url_destroy (&(mbox->url));
      if  (mbox->locker)
	locker_destroy (&(mbox->locker));
      if (mbox->auth)
	auth_destroy (&(mbox->auth), mbox);
      if(mbox->stream)
	stream_destroy (&(mbox->stream), mbox);
      unix_iunlock (mbox);
#ifdef HAVE_PTHREAD_H
      if (mbox->mutex)
	ptread_mutex_destroy (&(mbox->mutex));
#endif
      free (mbox);
      *pmbox = NULL;
    }
}

/* Open the file.  */
static int
unix_open (mailbox_t mbox, int flags)
{
  unix_data_t mud;
  int status = 0;

  if (mbox == NULL ||
      (mud = (unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* Authentication prologues.  */
  if (mbox->auth)
    auth_prologue (mbox->auth);

  /* Get a stream.  */
  if (mbox->stream == NULL)
    {
      /* FIXME: for small mbox we shout try to mmap ().  */

      status = (flags & MU_STREAM_CREAT) || (flags & MU_STREAM_APPEND);
      if (status == 0)
	status = mapfile_stream_create (&(mbox->stream));
      if (status != 0)
	{
	  status = file_stream_create (&(mbox->stream));
	  if (status != 0)
	    return status;
	}
      status = stream_open (mbox->stream, mbox->name, 0, flags);
      if (status != 0)
	return status;
    }
  else
    {
      status = stream_open (mbox->stream, mbox->name, 0, flags);
      if (status != 0)
	return status;
    }

  /* Authentication epilogue.  */
  if (mbox->auth)
    auth_epilogue (mbox->auth);

  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE, "unix_open(%s, %d)\n",
		 mbox->name, flags);

  /* Give an appopriate way to lock.  */
  if (mbox->locker == NULL)
    locker_create (&(mbox->locker), mbox->name, strlen (mbox->name),
		   MU_LOCKER_PID | MU_LOCKER_FCNTL);
  return 0;
}

static int
unix_close (mailbox_t mbox)
{
  unix_data_t mud;
  size_t i;

  if (mbox == NULL || (mud = (unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* Make sure we do not hold any lock for that file.  */
  unix_unlock (mbox);
  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE,  "unix_close(%s)\n",
		 mbox->name);

  /* Before closing we need to remove all the messages
     - to reclaim the memory
     - to prepare for another scan.  */
  for (i = 0; i < mud->umessages_count; i++)
    {
      unix_message_t mum = mud->umessages[i];
      /* Destroy the attach messages.  */
      if (mum == NULL)
	{
	  message_destroy (&(mum->message), mum);
	  free (mum);
	}
    }
  free (mud->umessages);
  mud->umessages = NULL;
  mud->messages_count = mud->umessages_count = 0;
  mud->size = 0;
  return stream_close (mbox->stream);
}

/* Mailbox Parsing.  */
#include "mbx_unixscan.c"

static int
unix_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE, "unix_scan(%s)\n", mbox->name);
  return unix_scan0 (mbox, msgno, pcount, 1);
}


/* FIXME:  How to handle a shrink ? meaning, the &^$^@%#@^& user start two
   browsers and delete files in one.  My views is that we should scream
   bloody murder and hunt them with a machette.  */
static int
unix_is_updated (mailbox_t mbox)
{
  off_t size;
  unix_data_t mud;
  if (mbox == NULL || (mud = (unix_data_t) mbox->data) == NULL)
    return EINVAL;
  if (stream_size (mbox->stream, &size) != 0)
    return 0;
  return (mud->size == size);
}

static int
unix_num_deleted (mailbox_t mbox, size_t *pnum)
{
  unix_data_t mud;
  unix_message_t mum;
  size_t i, total;
  if (mbox == NULL || (mud = (unix_data_t) mbox->data) == NULL)
    return EINVAL;
  for (i = total = 0; i < mud->messages_count; i++)
    {
      mum = mud->umessages[i];
      if (ATTRIBUTE_IS_DELETED (mum->new_flags))
	total++;
    }

  if (pnum)
    *pnum = total;
  return 0;
}

/* FIXME: the use of tmpfile() on some system can lead to race condition, We
   should use a safer approach. We take a very naive approach for this, it
   involves unfortunately two copies.  */
static FILE *
unix_tmpfile (mailbox_t mbox, char **pbox)
{
  const char *tmpdir;
  int fd;
  FILE *fp;
  unix_data_t mud = (unix_data_t)mbox->data;

  /*  P_tmpdir should be define in <stdio.h>.  */
#ifndef P_tmpdir
#  define P_tmpdir "/tmp"
#endif

  tmpdir =  getenv ("TEMPDIR") ? getenv ("TEMPDIR") : P_tmpdir;
  *pbox = calloc (1, strlen (tmpdir) + strlen ("MBOX_") +
		  strlen (mud->basename) + 1);
  if (*pbox == NULL)
    return NULL;
  sprintf (*pbox, "%s/%s%s", tmpdir, "MBOX_", mud->basename);

  /* FIXME:  I don't think this is the righ approach, creating an anonymous
     file would be better ?  no trace left behind.  */
  /* Create the file.  It must not exist.  If it does exist, fail.  */
  fd = open (*pbox, O_RDWR|O_CREAT|O_EXCL, 0600);
  if (fd < 0)
    {
      fprintf (stderr,"Can't create %s\n", *pbox);
      fprintf (stderr,"delete file <%s>, Please\n", *pbox);
      fprintf (stderr, "It was likely due to an error when expunging\n");
      return NULL;
    }
  fp = fdopen (fd, "w+");
  if (fp == 0)
    {
      close(fd);
      free (*pbox);
      *pbox = NULL;
    }

  /* Really I should just remove the file here.  */
  /* remove(*pbox); */
  return fp;
}

static int
unix_expunge (mailbox_t mbox)
{
  unix_data_t mud;
  unix_message_t mum;
  int status = 0;
  sigset_t sigset;
  FILE *tempfile;
  size_t nread;
  size_t i, j, dirty, first;
  off_t marker = 0;
  off_t total = 0;
  char buffer [BUFSIZ];
  char *tmpmbox = NULL;

  if (mbox == NULL || (mud = (unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* Noop.  */
  if (mud->messages_count == 0)
    return 0;

  /* Do we have a consistent view of the mailbox.  */
  /* FIXME: this is not enough, we can do better
     - by checking the file size and scream bloody murder if it has shrink.
     - if its bigger we should be able to handle it before the ftruncate ()
     by copying back the new messages.  */
  if (! unix_is_updated (mbox))
    {
      fprintf (stderr, "mailbox is not updated, try again.\n");
      return  EAGAIN;
    }

  /* Mark dirty the first mail with an attribute change.  */
  for (dirty = 0; dirty < mud->messages_count; dirty++)
    {
      mum = mud->umessages[dirty];
      if (mum->new_flags &&
	  ! ATTRIBUTE_IS_EQUAL (mum->old_flags, mum->new_flags))
	break;
    }

  /* Did something change ?  */
  if (dirty == mud->messages_count)
    return 0; /* Nothing change, bail out.  */

  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE, "unix_expunge (%s)\n",
		 mbox->name);

  /* This is redundant, we go to the loop again.  But it's more secure here
     since we don't want to be disturb when expunging.  */
  for (j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && mum->new_flags && ATTRIBUTE_IS_DELETED (mum->new_flags))
	message_destroy (&(mum->message), mum);
    }

  /* Create a tempory file.  */
  tempfile = unix_tmpfile (mbox, &tmpmbox);
  if (tempfile == NULL)
    return errno;

  /* Get the lock.  */
  if (unix_lock (mbox, MU_LOCKER_WRLOCK) < 0)
    {
      fclose (tempfile);
      remove (tmpmbox);
      free (tmpmbox);
      fprintf (stderr, "Failed to grab the lock\n");
      return ENOLCK;
    }

  /* Critical section, we can not allowed signal here.  */
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGTERM);
  sigaddset (&sigset, SIGHUP);
  sigaddset (&sigset, SIGTSTP);
  sigaddset (&sigset, SIGINT);
  sigaddset (&sigset, SIGWINCH);
  sigprocmask (SIG_BLOCK, &sigset, 0);

  unix_ilock (mbox, MU_LOCKER_RDLOCK);

  /* Set the marker position.  */
  total = marker = mud->umessages[dirty]->header_from;

  /* Copy to tempfile emails not mark deleted.  */
  for (first = 1, i = dirty; i < mud->messages_count; i++)
    {
      mum = mud->umessages[i];

      /* Skip it, if mark for deletion.  */
      if (ATTRIBUTE_IS_DELETED (mum->new_flags))
	continue;

      /* Add a NL separator between messages.  */
      if (first)
	first = 0;
      else
	{
	  fputc ('\n', tempfile);
	  total++;
	}

      /* Begining of header copy.  */
      {
	off_t current;
	/* This is done in two parts first we check if the attribute changed,
	   if yes then we have to update the "Status:" field.  Unfortunately
	   there is no requirement for the "Status:" to be the last field, so
	   we take the long approach; Copy up to the Status, update the
	   Status and copy the rest.  */

	/* Attribute change ?  */
	if (mum->new_flags
	    &&! ATTRIBUTE_IS_EQUAL (mum->old_flags, mum->new_flags)
	    && mum->header_status > mum->header_from)
	  {
	    size_t n = 0;
	    off_t offset = mum->header_from;
	    size_t len = mum->header_status - mum->header_from;
	    current = mum->header_status_end;
	    while (len > 0)
	      {
		nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
		if ((status = stream_read (mbox->stream, buffer,
					   nread, offset, &n) != 0)
		    || fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
		  {
		    if (status == 0)
		      status = errno;
		    fprintf (stderr, "expunge:%d: (%s)\n", __LINE__,
			     strerror (status));
		    goto bailout;
		  }
		len -= n;
		total += n;
		offset += n;
	      }

	    /* Put the new attributes.  */
	    {
	      char abuf[64];
	      size_t na = 0;
	      abuf[0] = '\0';
	      flags_to_string (mum->new_flags, abuf, sizeof(abuf), &na);
	      fputs (abuf, tempfile);
	      total += na;
	    }
	  }
	else /* Attribute did not change.  */
	  current = mum->header_from;

	/* Copy the rest of header without changes.  */
	{
	  size_t n = 0;
	  off_t offset = current;
	  size_t len = mum->body - current;
	  while (len > 0)
	    {
	      nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	      if ((status = stream_read (mbox->stream, buffer, nread,
					 offset, &n) != 0)
		  || fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
		{
		  if (status == 0)
		    status = errno;
		  fprintf (stderr, "expunge:%d: %s", __LINE__,
			   strerror (status));
		  goto bailout;
		}
	      len -= n;
	      total += n;
	      offset += n;
	    }
	}
      } /* End of header copy.  */

      /* Copy the body.  */
      {
	size_t n = 0;
	off_t offset = mum->body;
	size_t len = mum->body_end - mum->body;
	while (len > 0)
	  {
	    nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	    if ((status = stream_read (mbox->stream, buffer, nread,
				       offset, &n) != 0)
		|| fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
	      {
		if (status == 0)
		  status = errno;
		fprintf (stderr, "expunge:%d: %s", __LINE__,
			 strerror (status));
		goto bailout;
	      }
	    len -= n;
	    total += n;
	    offset += n;
	  }
      } /* End of body copy.  */
    } /* for (;;) */

  /* Caution: before ftruncate()ing the file see if we've receiving new mail
     Some program may not respect the lock, or the lock was held for too
     long. */
  {
    off_t size = 0;
    if (stream_size (mbox->stream, &size) == 0)
      {
	size_t n = 0;
	off_t offset = size;
	size_t len = size - mud->size;
	while (len > 0)
	  {
	    nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	    if ((status = stream_read (mbox->stream, buffer, nread,
				       offset, &n) != 0)
		|| fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
	      {
		if (status == 0)
		  status = errno;
		fprintf (stderr, "expunge:%d: %s", __LINE__,
			 strerror (status));
		goto bailout;
	      }
	    len -= n;
	    total += n;
	    offset += n;
	  }
      }
  } /* End of precaution.  */

  /* Seek and rewrite it.  */
  rewind (tempfile);
  if (total > 0)
    {
      size_t n = 0;
      off_t offset = marker;
      while ((nread = fread (buffer, sizeof (*buffer),
			     sizeof (buffer), tempfile)) != 0)
	{
	  while (nread)
	    {
	      status = stream_write (mbox->stream, buffer, nread, offset, &n);
	      if (status != 0)
		goto bailout;
	      nread -= n;
	      offset += n;
	    }
	}
    }

  /* How can I handle error here ??  */
  clearerr (tempfile);

  /* Flush/truncation.  */
  stream_flush (mbox->stream);
  status = stream_truncate (mbox->stream, total);
  if (status != 0)
    {
      fprintf (stderr, "expunge:%d: %s", __LINE__,
	       strerror (status));
      goto bailout;
    }

  /* Don't remove the tmp mbox in case of errors.  */
  remove (tmpmbox);

 bailout:

  free (tmpmbox);
  /* Release the locks.  */
  unix_unlock (mbox);
  unix_iunlock (mbox);
  fclose (tempfile);
  sigprocmask (SIG_UNBLOCK, &sigset, 0);

  /* We need to readjust the pointers.  */
  if (status == 0)
    {
      size_t dlast;
      for (j = dirty, dlast = mud->messages_count - 1;
	   j < mud->messages_count; j++)
	{
	  /* Clear all the references, any attach messages been already
	     destroy above.  */
	  mum = mud->umessages[j];
	  if (mum->new_flags && ATTRIBUTE_IS_DELETED (mum->new_flags))
	    {
	      memmove (mud->umessages + j, mud->umessages + j + 1,
		       (dlast - dirty) * sizeof (mum));
	      mum->header_from = mum->header_from_end = 0;
	      mum->header_status = mum->header_status_end = 0;
	      mum->body = mum->body_end = 0;
	      mum->header_lines = mum->body_lines = 0;
	      mud->umessages[dlast] = mum;
	      dlast--;
	      mum = mud->umessages[j];
	    }
	  mum->header_from = mum->header_from_end = 0;
	  mum->header_status = mum->header_status_end = 0;
	  mum->body = mum->body_end = 0;
	  mum->header_lines = mum->body_lines = 0;
	}
      /* This is should reset the messages_count, the last * argument 0 means
	 not to send event notification.  */
      unix_scan0 (mbox, dirty, NULL, 0);
    }
  return status;
}

static int
unix_get_fd (stream_t is, int *pfd)
{
  unix_message_t mum;

  if (is == NULL || (mum = is->owner) == NULL)
    return EINVAL;

  return stream_get_fd (mum->stream, pfd);
}

static int
unix_get_flags (attribute_t attr, int *pflags)
{
  unix_message_t mum;

  if (attr == NULL || (mum = attr->owner) == NULL)
    return EINVAL;

  if (pflags)
    *pflags = mum->new_flags;
  return 0;
}

static int
unix_set_flags (attribute_t attr, int flags)
{
  unix_message_t mum;

  if (attr == NULL || (mum = attr->owner) == NULL)
    return EINVAL;

  mum->new_flags |= flags;
  return 0;
}

static int
unix_unset_flags (attribute_t attr, int flags)
{
  unix_message_t mum;

  if (attr == NULL || (mum = attr->owner) == NULL)
    return EINVAL;

  mum->new_flags &= flags;
  return 0;
}

static int
unix_readstream (stream_t is, char *buffer, size_t buflen,
		 off_t off, size_t *pnread)
{
  unix_message_t mum;
  size_t nread = 0;

  if (is == NULL || (mum = (unix_message_t)is->owner) == NULL)
    return EINVAL;

  if (buffer == NULL || buflen == 0)
    {
      if (pnread)
	*pnread = nread;
      return 0;
    }

  {
    off_t ln = mum->body_end - (mum->body + off);
    size_t n = 0;
    int status;
    if (ln > 0)
      {
	nread = ((size_t)ln < buflen) ? ln : buflen;
	/* Position the file pointer and the buffer.  */
	status = stream_read (mum->stream, buffer, nread, mum->body + off, &n);
	if (status != 0)
	  return status;
      }
  }

  if (pnread)
    *pnread = nread;
  return 0;
}

static int
unix_get_header_read (stream_t is, char *buffer, size_t len,
		      off_t off, size_t *pnread)
{
  unix_message_t mum;
  size_t nread = 0;
  int status = 0;
  off_t ln;

  if (is == NULL || (mum = is->owner) == NULL)
    return EINVAL;

  ln = mum->body - (mum->header_from_end + off);
  if (ln > 0)
    {
      nread = ((size_t)ln < len) ? ln : len;
      /* Position the file pointer and the buffer.  */
      status = stream_read (mum->stream, buffer, nread,
			    mum->header_from_end + off, &nread);
    }
  if (pnread)
    *pnread = nread;
  return status;
}

static int
unix_body_size (body_t body, size_t *psize)
{
  unix_message_t mum = body->owner;
  if (mum == NULL)
    return EINVAL;
  if (psize)
    *psize = mum->body_end - mum->body;
  return 0;
}

static int
unix_body_lines (body_t body, size_t *plines)
{
  unix_message_t mum = body->owner;
  if (mum == NULL)
    return EINVAL;
  if (plines)
    *plines = mum->body_lines;
  return 0;
}

static int
unix_msg_received (message_t msg, char *buf, size_t len,
			   size_t *pnwrite)
{
  unix_message_t mum = msg->owner;
  size_t n = 0;
  int status;
  char buffer[512];

  if (mum == NULL)
    return EINVAL;

  status = stream_readline (mum->stream, buffer, sizeof(buffer),
			    mum->header_from, &n);
  if (status != 0)
    {
      if (pnwrite)
	*pnwrite = 0;
      if (buf)
	*buf = '\0';
      return status;
    }

  if (n > 5)
    {
      char *s = strchr (buffer + 5, ' ');
      if (s)
	{
	  if (buf && len > 0)
	    {
	      strncpy (buf, s + 1, len);
	      buffer [len - 1] = '\0';
	    }
	  if (pnwrite)
	    *pnwrite = strlen (s + 1);
	  return 0;
	}
    }
  if (pnwrite)
    *pnwrite = 0;
  if (buf)
    *buf = '\0';
  return 0;
}

static int
unix_msg_from (message_t msg, char *buf, size_t len, size_t *pnwrite)
{
  unix_message_t mum = msg->owner;
  size_t n = 0;
  int status;
  char buffer[512];

  if (mum == NULL)
    return EINVAL;

  status = stream_readline (mum->stream, buffer, sizeof(buffer),
			    mum->header_from, &n);
  if (status != 0)
    {
      if (pnwrite)
	*pnwrite = 0;
      if (buf)
	*buf = '\0';
      return status;
    }

  if (n > 5)
    {
      char *s = strchr (buffer + 5, ' ');
      if (s)
	{
	  *s = '\0';
	  if (buf && len > 0)
	    {
	      strncpy (buf, buffer + 5, len);
	      buffer [len - 1] = '\0';
	    }
	  if (pnwrite)
	    *pnwrite = strlen (buffer + 5);
	  return 0;
	}
    }
  if (pnwrite)
    *pnwrite = 0;
  if (buf)
    *buf = '\0';
  return 0;
}

static int
unix_get_message (mailbox_t mbox, size_t msgno, message_t *pmsg)
{
  int status;
  unix_data_t mud;
  unix_message_t mum;
  message_t msg = NULL;

  /* Sanity checks.  */
  if (mbox == NULL || pmsg == NULL || (mud = (unix_data_t)mbox->data) == NULL
      || (!(mud->messages_count > 0 && msgno > 0
	    && msgno <= mud->messages_count)))
    return EINVAL;

  mum = mud->umessages[msgno - 1];

  /* Check if we already have it.  */
  if (mum->message)
    {
      if (pmsg)
	*pmsg = mum->message;
      return 0;
    }

  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE,
		 "unix_get_message(%s, %d)\n", mbox->name, msgno);

  /* Get an empty message struct.  */
  status = message_create (&msg, mum);
  if (status != 0)
    return status;

  /* Set the header.  */
  {
    header_t header;
    stream_t stream;
    if ((status = header_create (&header, NULL, 0, mum)) != 0
	|| (status = stream_create (&stream, MU_STREAM_READ, mum)) != 0)
      {
	message_destroy (&msg, mum);
	return status;
      }
    stream_set_read (stream, unix_get_header_read, mum);
    stream_set_fd (stream, unix_get_fd, mum);
    stream_set_flags (stream, MU_STREAM_READ, mum);
    header_set_stream (header, stream, mum);
    message_set_header (msg, header, mum);
  }

  /* Set the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, mum);
    if (status != 0)
      {
	message_destroy (&msg, mum);
	return status;
      }
    mum->new_flags = mum->old_flags;
    attribute_set_get_flags (attribute, unix_get_flags, mum);
    attribute_set_set_flags (attribute, unix_set_flags, mum);
    attribute_set_unset_flags (attribute, unix_unset_flags, mum);
    message_set_attribute (msg, attribute, mum);
  }

  /* Prepare the body.  */
  {
    body_t body;
    stream_t stream;
    int flags = MU_STREAM_READ;
    if ((status = body_create (&body, mum)) != 0
	|| (status = stream_create (&stream, flags, mum)) != 0)
      {
	message_destroy (&msg, mum);
	return status;
      }
    stream_set_read (stream, unix_readstream, mum);
    stream_set_fd (stream, unix_get_fd, mum);
    stream_get_flags (mbox->stream, &flags);
    stream_set_flags (stream, flags, mum);
    body_set_stream (body, stream, mum);
    body_set_size (body, unix_body_size, mum);
    body_set_lines (body, unix_body_lines, mum);
    message_set_body (msg, body, mum);
  }

  /* Set the envelope.  */
  message_set_from (msg, unix_msg_from, mum);
  message_set_received (msg, unix_msg_received, mum);

  /* Attach the message to the mailbox unix data.  */
  mum->message = msg;

  if (pmsg)
    *pmsg = msg;
  return 0;
}

static int
unix_append_message (mailbox_t mbox, message_t msg)
{
  unix_data_t mud;
  if (mbox == NULL || msg == NULL ||
      (mud = (unix_data_t)mbox->data) == NULL)
    return EINVAL;

  mailbox_debug (mbox, MU_MAILBOX_DEBUG_TRACE,
		 "unix_append_message (%s)\n", mbox->name);

  unix_lock (mbox, MU_LOCKER_WRLOCK);
  {
    off_t size;
    int status;
    size_t n = 0;
    char nl = '\n';

    /* Move to the end of the file, not necesary if _APPEND mode.  */
    status = stream_size (mbox->stream, &size);
    if (status != 0)
      {
	unix_unlock (mbox);
	return status;
      }

    /* Generate a "From " separator.  */
    {
      char from[128];
      char date[128];
      char *s;
      size_t f = 0, d = 0;
      *date = *from = '\0';
      message_from (msg, from, sizeof (from), &f);
      s = memchr (from, nl, f);
      if (s)
	{
	  *s = '\0';
	  f--;
	}
      message_received (msg, date, sizeof (date), &d);
      s = memchr (date, nl, d);
      if (s)
	{
	  *s = '\0';
	  d--;
	}
      stream_write (mbox->stream, "From ", 5, size, &n); size += n;
      stream_write (mbox->stream, from, f, size, &n); size += n;
      stream_write (mbox->stream, " ", 1, size, &n); size += n;
      stream_write (mbox->stream, date, d, size, &n); size += n;
      stream_write (mbox->stream, &nl , 1, size, &n); size += n;
    }

    /* Append the Message.  */
    {
      char buffer[BUFSIZ];
      size_t nread = 0;
      off_t off = 0;
      stream_t is;
      message_get_stream (msg, &is);
      do
	{
	  stream_read (is, buffer, sizeof (buffer), off, &nread);
	  stream_write (mbox->stream, buffer, nread, size, &n);
	  off += nread;
	  size += n;
	}
      while (nread > 0);
      stream_write (mbox->stream, &nl, 1, size, &n);
    }
  }
  stream_flush (mbox->stream);
  unix_unlock (mbox);
  return 0;
}

static int
unix_size (mailbox_t mbox, off_t *psize)
{
  off_t size;
  int status;

  if (mbox == NULL)
    return EINVAL;

  /* Maybe was not open yet ??  */
  status  = stream_size (mbox->stream, &size);
  if (status != 0)
    return status;
  if (psize)
    *psize = size;
  return 0;
}

static int
unix_messages_count (mailbox_t mbox, size_t *pcount)
{
  unix_data_t mud;
  if (mbox == NULL || (mud = (unix_data_t) mbox->data) == NULL)
    return EINVAL;

  if (! unix_is_updated (mbox))
    return unix_scan0 (mbox,  1, pcount, 1);

  if (pcount)
    *pcount = mud->messages_count;

  return 0;
}

/* Locking.  */
static int
unix_lock (mailbox_t mbox, int flag)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_lock (locker, flag);
    }
  return 0;
}

static int
unix_touchlock (mailbox_t mbox)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_touchlock (locker);
    }
  return 0;
}

static int
unix_unlock (mailbox_t mbox)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_unlock (locker);
    }
  return 0;
}

static int
unix_ilock (mailbox_t mbox, int flag)
{
#ifdef HAVE_PTHREAD_H
  (void)flag; /* We should use rwlocks for more concurency.  */
  if (mbox == NULL)
    return EINVAL;
  return pthread_mutex_lock (&(mbox->mutex));
#else
  (void)mbox; (void)flag;
  return 0;
#endif
}

static int
unix_iunlock (mailbox_t mbox)
{
#ifdef HAVE_PTHREAD_H
  if (mbox == NULL)
    return EINVAL;
  return pthread_mutex_unlock (&(mbox->mutex));
#else
  (void)mbox;
  return 0;
#endif
}
