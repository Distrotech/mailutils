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
#include <errno.h>
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

static int mailbox_unix_create (mailbox_t *pmbox, const char *name);
static void mailbox_unix_destroy (mailbox_t *pmbox);

struct mailbox_registrar _mailbox_unix_registrar =
{
  "UNIX MBOX",
  mailbox_unix_create, mailbox_unix_destroy
};

/*
 * Keep the position of where the header and body starts
 * and ends. old_attr is the "Status:" message.
 */
typedef struct _mailbox_unix_message
{
  /* offset of the parts of the messages in the mailbox*/
  off_t header_from;
  off_t header_from_end;
  /* little hack to make things easier
   * when updating the attribute
   */
  off_t header_status;
  off_t header_status_end;
  off_t body;
  off_t body_end;

  /* old_attr contains the definition of Header: */
  /* new_attr what we want when expunging */
  attribute_t old_attr;
  attribute_t new_attr;

  size_t header_lines;
  size_t body_lines;
  stream_t stream;

  /* if we have a message attach to it */
  message_t message;

} *mailbox_unix_message_t;

/*
 * umessages is an array of pointers that contains umessages_count
 * of mailbox_unix_message_t*; umessages[umessages_count].
 * We do it this because realloc() can move everything to
 * a new memory region and invalidating all the pointers someone
 * has on the messages.  Thanks to <Dave Inglis> for pointing this out.
 * messages_count is the count number of messages parsed so far.
 */
typedef struct _mailbox_unix_data
{
  mailbox_unix_message_t *umessages;
  size_t umessages_count;
  size_t messages_count;
  stream_t stream;
  char *dirname;
  char *basename;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  off_t size;

} *mailbox_unix_data_t;

static int mailbox_unix_open (mailbox_t mbox, int flag);
static int mailbox_unix_close (mailbox_t mbox);

static int mailbox_unix_get_message (mailbox_t, size_t msgno, message_t *msg);
static int mailbox_unix_append_message (mailbox_t, message_t msg);
static int mailbox_unix_messages_count (mailbox_t, size_t *msgno);
static int mailbox_unix_expunge (mailbox_t);
static int mailbox_unix_num_deleted (mailbox_t, size_t *);

static int mailbox_unix_scan0 (mailbox_t, size_t count, size_t *pcount, int);
static int mailbox_unix_scan (mailbox_t, size_t count, size_t *pcount);
static int mailbox_unix_is_updated (mailbox_t);

static int mailbox_unix_size (mailbox_t, off_t *size);


/* private stuff */

static int mailbox_unix_get_header (mailbox_unix_message_t mum, char *buffer,
				    size_t len, off_t off, ssize_t *pnread);
static int mailbox_unix_getfd (stream_t is, int *pfd);
static int mailbox_unix_readstream (stream_t is, char *buffer, size_t buflen,
				    off_t off, size_t *pnread);
static int mailbox_unix_body_size (body_t body, size_t *psize);
static int mailbox_unix_body_lines (body_t body, size_t *plines);
static int mailbox_unix_msg_from (message_t msg, char *buf, size_t len,
				  size_t *pnwrite);
static int mailbox_unix_msg_received (message_t msg, char *buf, size_t len,
				      size_t *pnwrite);
static int mailbox_unix_lock (mailbox_t mbox, int flag);
static int mailbox_unix_touchlock (mailbox_t mbox);
static int mailbox_unix_unlock (mailbox_t mbox);
static int mailbox_unix_ilock (mailbox_t mbox, int flag);
static int mailbox_unix_iunlock (mailbox_t mbox);

/* We allocate the mailbox_t struct, but don't do any
 * parsing on the name or even test for existence.
 * However we do strip any leading "unix:" part of
 * the name, this is suppose to be the protocol/scheme name.
 * Hopefully there will not be a mailbox name "unix:"
 */
static int
mailbox_unix_create (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;
  mailbox_unix_data_t mud;
  const char *sep;
  size_t name_len;
  int i;

  /* sanity check */
  if (name == NULL || *name == '\0')
    return EINVAL;

  name_len = strlen (name);

#define UNIX_SCHEME "unix:"
#define UNIX_SCHEME_LEN 5
#define SEPARATOR '/'

  /* skip the url scheme */
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

  /* allocate memory for mbox */
  mbox = calloc (1, sizeof (*mbox));
  if (mbox == NULL)
    return ENOMEM;

  /* allocate specific unix mbox data */
  mud = mbox->data = calloc (1, sizeof (*mud));
  if (mbox->data == NULL)
    {
      mailbox_unix_destroy (&mbox);
      return ENOMEM;
    }

  /* copy the name */
  mbox->name = calloc (name_len + 1, sizeof (char));
  if (mbox->name == NULL)
    {
      mailbox_unix_destroy (&mbox);
      return ENOMEM;
    }
  memcpy (mbox->name, name, name_len);

  /* save the basename and dirname
   * So we split the name.  But this should probably be
   * supported via "maildir:" or the mailbox mgr.
   */
  /* equivalent to strrchr (name, '/'); */
  for (i = name_len, sep = NULL; i >= 0; i--)
    {
      /* break on the first separator */
      if (name[i] == SEPARATOR)
	{
	  sep = &(name[i]);
	  break;
	}
    }

  if (sep)
    {
      /* split it into two */
      mud->dirname = calloc ((sep - name) + 1, sizeof (char));
      if (mud->dirname == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->dirname, name, sep - name);

      ++sep;
      mud->basename = calloc (name_len - (sep - name) + 1, sizeof (char));
      if (mud->basename == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->basename, sep, name_len - (sep - name));
    }
  else
    {
      /* use the relative directory "." */
      /* FIXME: should we call getcwd() instead ? */
      mud->dirname = calloc (2 , sizeof (char));
      if (mud->dirname == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      mud->dirname[0] = '.';

      mud->basename = calloc (name_len + 1, sizeof (char));
      if (mud->basename == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->basename, name, name_len);
    }

#ifdef HAVE_PHTREAD_H
  /* mutex when accessing the structure fields */
  /* FIXME: should we use rdwr locks instead ?? */
  pthread_mutex_init (&(mbox->mutex), NULL);
#endif

  mbox->_create = mailbox_unix_create;
  mbox->_destroy = mailbox_unix_destroy;

  mbox->_open = mailbox_unix_open;
  mbox->_close = mailbox_unix_close;

  /* messages */
  mbox->_get_message = mailbox_unix_get_message;
  mbox->_append_message = mailbox_unix_append_message;
  mbox->_messages_count = mailbox_unix_messages_count;
  mbox->_expunge = mailbox_unix_expunge;
  mbox->_num_deleted = mailbox_unix_num_deleted;

  mbox->_scan = mailbox_unix_scan;
  mbox->_is_updated = mailbox_unix_is_updated;

  mbox->_size = mailbox_unix_size;

  (*pmbox) = mbox;

  return 0; /* okdoke */
}

static void
mailbox_unix_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mailbox_t mbox = *pmbox;
      mailbox_unix_close (mbox);
      if (mbox->data)
	{
	  size_t i;
	  mailbox_unix_data_t mud = mbox->data;
	  free (mud->dirname);
	  free (mud->basename);
	  for (i = 0; i < mud->umessages_count; i++)
	    {
	      mailbox_unix_message_t mum = mud->umessages[i];
	      if (mum == NULL)
		continue;
	      /* Destroy the attach messages */
	      attribute_destroy (&(mum->old_attr));
              message_destroy (&(mum->message), mum);
	      /* new_attr free by message_destroy() */
	      /* attribute_destroy (&(mum->new_attr)); */
	      free (mum);
	    }
	  free (mud->umessages);
	  free (mbox->data);
	}
      free (mbox->name);
      /* free the event array */
      free (mbox->event);
      /* destroy the url */
      if (mbox->url)
	url_destroy (&(mbox->url));
      if  (mbox->locker)
	locker_destroy (&(mbox->locker));
      if (mbox->auth)
	auth_destroy (&(mbox->auth), (*pmbox));
      if(mbox->stream)
	stream_destroy (&(mbox->stream), *pmbox);
      mailbox_unix_iunlock (mbox);
#ifdef HAVE_PTHREAD_H
      if (mbox->mutex)
	ptread_mutex_destroy (&(mbox->mutex));
#endif
      free (*pmbox);
      *pmbox = NULL;
    }
}

/* Open the file. */
static int
mailbox_unix_open (mailbox_t mbox, int flags)
{
  mailbox_unix_data_t mud;
  int status;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* Authentication prologues */
  if (mbox->auth)
    auth_prologue (mbox->auth);

  /* get a stream */
  if (mbox->stream == NULL)
    {
      status = file_stream_create (&(mbox->stream));
      if (status != 0)
	return status;
    }
  status = stream_open (mbox->stream, mbox->name, 0, flags);
  if (status != 0)
    {
      stream_destroy (&(mbox->stream), mbox);
      return status;
    }

  /* Authentication */
  if (mbox->auth)
    {
      char *user = NULL;
      char *passwd = NULL;
      status = auth_authenticate (mbox->auth, &user, &passwd);
      free (user);
      free (passwd);
      if (status != 0)
	return status;
    }

  /* Authentication epilogues */
  if (mbox->auth)
    auth_epilogue (mbox->auth);

  /* give an appopriate way to lock */
  if (mbox->locker == NULL)
    locker_create (&(mbox->locker), mbox->name,
		   strlen (mbox->name), MU_LOCKER_PID | MU_LOCKER_FCNTL);
  return 0;
}

static int
mailbox_unix_close (mailbox_t mbox)
{
  mailbox_unix_data_t mud;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  stream_close (mbox->stream);
  /* make sure we do not hold any lock for that file */
  mailbox_unix_unlock (mbox);
  return 0;
}

/* Mailbox Parsing */
#include "mbx_unixscan.c"

static int
mailbox_unix_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
  return mailbox_unix_scan0 (mbox, msgno, pcount, 1);
}


/* FIXME:  How to handle a shrink ? meaning, the &^$^@%#@^& user
 * start two browsers and delete files in one.  My views
 * is that we should scream bloody murder and hunt them with
 * a machette.
 */
static int
mailbox_unix_is_updated (mailbox_t mbox)
{
  off_t size;
  mailbox_unix_data_t mud;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;
  if (stream_size (mbox->stream, &size) != 0)
    return 0;
  return (mud->size == size);
}

static int
mailbox_unix_num_deleted (mailbox_t mbox, size_t *pnum)
{
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;
  size_t i, total;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;
  for (i = total = 0; i < mud->messages_count; i++)
    {
      mum = mud->umessages[i];
      if (attribute_is_deleted (mum->new_attr))
	total++;
    }

  if (pnum)
    *pnum = total;
  return 0;
}

/*
  FIXME: the use of tmpfile() on some system can lead to
  race condition, We should use a safer approach.
  We take a very naive approach for this, it involves unfortunately
  two copies.
*/
static FILE *
mailbox_unix_tmpfile (mailbox_t mbox, char **pbox)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t)mbox->data;
  int fd;
  FILE *fp;

#ifndef P_tmpdir
# define P_tmpdir "/tmp"
#endif

  *pbox = calloc (1, strlen (P_tmpdir) + strlen ("MBOX_") +
		   strlen (mud->basename) + 1);
  if (*pbox == NULL)
    return NULL;
  sprintf (*pbox, "%s/%s%s", P_tmpdir, "MBOX_", mud->basename);

  /* FIXME:  I don't think this is the righ approach
   * Creating an anonymous file would be better ?
   * no trace left behind.
   */
  /* Create the file.  It must not exist.  If it does exist, fail. */
  fd = open(*pbox, O_RDWR|O_CREAT|O_EXCL, 0600);
  if (fd < 0)
    {
      fprintf(stderr,"Can't create %s\n", *pbox);
      fprintf(stderr,"delete file <%s>, Please\n", *pbox);
      return NULL;
    }
  fp = fdopen(fd, "w+");
  if (fp == 0)
    {
      close(fd);
      free (*pbox);
      *pbox = NULL;
    }

  /* really I should just remove the file here */
  /* remove(*pbox); */
  return fp;
}

static int
mailbox_unix_expunge (mailbox_t mbox)
{
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;
  int status = 0;
  sigset_t sigset;
  FILE *tempfile;
  size_t nread;
  size_t i, j, dirty, first;
  off_t marker = 0;
  off_t total = 0;
  char buffer [BUFSIZ];
  char *tmpmbox = NULL;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* noop */
  if (mud->messages_count == 0)
    return 0;

  /* Do we have a consistent view of the mailbox */
  /* FIXME: this is not enough, we can do better
   * - by checking the file size and scream bloody murder
   * if it has shrink.
   * - if its bigger we should be able to handle it
   * before the ftruncate() by copying back the new messages.
   */
  if (! mailbox_unix_is_updated (mbox))
    return  EAGAIN;

  /* mark dirty the first mail with an attribute change */
  for (dirty = 0; dirty < mud->messages_count; dirty++)
    {
      mum = mud->umessages[dirty];
      if (mum->new_attr &&
	  ! attribute_is_equal (mum->old_attr, mum->new_attr))
	break;
    }

  /* did something change ? */
  if (dirty == mud->messages_count)
    return 0; /* nothing change, bail out */

  /* Send notification to all the listeners
   * this is redundant, we go to the loop again
   * But it's more secure here since we don't
   * want to be disturb when expunging.
   */
  for (j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && mum->new_attr && attribute_is_deleted (mum->new_attr))
	{
	  attribute_t attr;
	  attribute_create (&attr);
	  attribute_copy (attr, mum->new_attr);
	  /* Hack: message_destroy() will free() the attribute, save it */
	  message_destroy (&(mum->message), mum);
	  mum->new_attr = attr;
	}
    }

  /* create a tempory file */
  tempfile = mailbox_unix_tmpfile (mbox, &tmpmbox);
  if (tempfile == NULL)
    return errno;

  /* Get the lock */
  if (mailbox_unix_lock (mbox, MU_LOCKER_WRLOCK) < 0)
    {
      fclose (tempfile);
      remove (tmpmbox);
      free (tmpmbox);
      return ENOLCK;
    }

  /* Critical section, we can not allowed signal here */
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGTERM);
  sigaddset (&sigset, SIGHUP);
  sigaddset (&sigset, SIGTSTP);
  sigaddset (&sigset, SIGINT);
  sigaddset (&sigset, SIGWINCH);
  sigprocmask (SIG_BLOCK, &sigset, 0);

  mailbox_unix_ilock (mbox, MU_LOCKER_RDLOCK);

  /* set the marker position */
  total = marker = mud->umessages[dirty]->header_from;

  /* copy to tempfile emails not mark deleted */
  for (first = 1, i = dirty; i < mud->messages_count; i++)
    {
      mum = mud->umessages[i];

      /* skip it, if mark for deletion */
      if (attribute_is_deleted (mum->new_attr))
	continue;

      /* add a NL separator between messages */
      if (first)
	first = 0;
      else
	{
	  fputc ('\n', tempfile);
	  total++;
	}

      /* Begining of header copy */
      {
	off_t current;
	/* This is done in two parts first we check if the attribute
	 * changed, if yes then we have to update the "Status:" field.
	 * Unfortunately there is no requirement for the "Status:"
	 * to be the last field, so we take the long approach;
	 * Copy up to the Status, update the Status and copy the rest.
	 */

	/* attribute change ? */
	if (mum->new_attr
	    &&! attribute_is_equal (mum->old_attr, mum->new_attr)
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
		    goto bailout;
		  }
		len -= n;
		total += n;
		offset += n;
	      }

	    /* put the new attributes */
	    {
	      char abuf[64];
	      size_t na = 0;
	      abuf[0] = '\0';
	      attribute_to_string (mum->new_attr, abuf, sizeof(abuf), &na);
	      fputs (abuf, tempfile);
	      total += na;
	    }
	  }
	else /* attribute did not change */
	  current = mum->header_from;

	/* copy the rest of header without changes */
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
		  goto bailout;
		}
	      len -= n;
	      total += n;
	      offset += n;
	    }
	}
      } /* end of header copy */

      /* copy the body */
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
		goto bailout;
	      }
	    len -= n;
	    total += n;
	    offset += n;
	  }
      } /* end of body copy */
    } /* for (;;) */

  /*
   * Caution:
   * before ftruncate()ing the file see if we've receiving new mail
   * Some program may not respect the lock, or the lock was held
   * for too long.
   */
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
		goto bailout;
	      }
	    len -= n;
	    total += n;
	    offset += n;
	  }
      }
  } /* end of precaution */

  /* Seek and rewrite it */
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

  /* how can I handle error here ?? */
  clearerr (tempfile);

  /* flush/truncation */
  status = stream_truncate (mbox->stream, total);
  if (status != 0)
    goto bailout;
  stream_flush (mbox->stream);

  /* Don't remove the tmp mbox in case of errors */
  remove (tmpmbox);

 bailout:

  free (tmpmbox);
  /* Release the locks */
  mailbox_unix_unlock (mbox);
  mailbox_unix_iunlock (mbox);
  fclose (tempfile);
  sigprocmask (SIG_UNBLOCK, &sigset, 0);

  /* we need to readjust the pointers */
  if (status == 0)
    {
      size_t dlast;
      for (j = dirty, dlast = mud->messages_count - 1;
	   j < mud->messages_count; j++)
	{
	  /* clear all the references,
	   * any attach messages been already destroy above
	   */
	  mum = mud->umessages[j];
	  if (mum->new_attr && attribute_is_deleted (mum->new_attr))
	    {
	      memmove (mud->umessages + j, mud->umessages + j + 1,
		       (dlast - dirty) * sizeof (mum));
	      mum->header_from = mum->header_from_end = 0;
	      mum->header_status = mum->header_status_end = 0;
	      mum->body = mum->body_end = 0;
	      mum->header_lines = mum->body_lines = 0;
	      attribute_destroy (&(mum->new_attr));
	      mud->umessages[dlast] = mum;
	      dlast--;
	      mum = mud->umessages[j];
	    }
	  mum->header_from = mum->header_from_end = 0;
	  mum->header_status = mum->header_status_end = 0;
	  mum->body = mum->body_end = 0;
	  mum->header_lines = mum->body_lines = 0;
	}
      mailbox_unix_scan0 (mbox, dirty, NULL, 0);
    }
  return status;
}

static int
mailbox_unix_getfd (stream_t is, int *pfd)
{
  mailbox_unix_message_t mum;

  if (is == NULL || (mum = is->owner) == NULL)
    return EINVAL;

  stream_get_fd (mum->stream, pfd);
  return 0;
}

static int
mailbox_unix_readstream (stream_t is, char *buffer, size_t buflen,
			off_t off, size_t *pnread)
{
  mailbox_unix_message_t mum;
  size_t nread = 0;

  if (is == NULL || (mum = (mailbox_unix_message_t)is->owner) == NULL)
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
	/* position the file pointer and the buffer */
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
mailbox_unix_get_header (mailbox_unix_message_t mum, char *buffer,
			 size_t len, off_t off, ssize_t *pnread)
{
  size_t nread = 0;
  size_t n = 0;
  int status;
  off_t ln = mum->body - (mum->header_from_end + off);
  if (ln > 0)
    {
      nread = ((size_t)ln < len) ? ln : len;
      /* position the file pointer and the buffer */
      status = stream_read (mum->stream, buffer, nread,
			    mum->header_from_end + off, &n);
      if (status != 0)
	return status;
    }
  *pnread = nread;
  return 0;
}

static int
mailbox_unix_body_size (body_t body, size_t *psize)
{
  mailbox_unix_message_t mum = body->owner;
  if (mum == NULL)
    return EINVAL;
  if (psize)
    *psize = mum->body_end - mum->body;
  return 0;
}

static int
mailbox_unix_body_lines (body_t body, size_t *plines)
{
  mailbox_unix_message_t mum = body->owner;
  if (mum == NULL)
    return EINVAL;
  if (plines)
    *plines = mum->body_lines;
  return 0;
}

static int
mailbox_unix_msg_received (message_t msg, char *buf, size_t len,
			   size_t *pnwrite)
{
  mailbox_unix_message_t mum = msg->owner;
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
mailbox_unix_msg_from (message_t msg, char *buf, size_t len, size_t *pnwrite)
{
  mailbox_unix_message_t mum = msg->owner;
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
mailbox_unix_get_message (mailbox_t mbox, size_t msgno, message_t *pmsg)
{
  int status;
  ssize_t nread;
  char *pbuf = NULL;
  char *tbuf = NULL;
  char buf[BUFSIZ];
  off_t offset = 0;
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;
  message_t msg = NULL;
  stream_t stream = NULL;
  header_t header = NULL;
  body_t body = NULL;
  int flags = 0;

  if (mbox == NULL || pmsg == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL ||
      (!(mud->messages_count > 0 && msgno > 0 &&
	 msgno <= mud->messages_count)))
    return EINVAL;

  mum = mud->umessages[msgno - 1];

  /* check if we already have it */
  if (mum->message)
    {
      if (pmsg)
	*pmsg = mum->message;
      return 0;
    }

  /* get the headers */
  do
    {
      status = mailbox_unix_get_header (mum, buf, sizeof(buf), offset, &nread);
      if (status != 0)
	{
	  free (pbuf);
	  return status;
	}

      if (nread == 0)
	break;

      tbuf = realloc (pbuf, offset + nread);
      if (tbuf == NULL)
	{
	  free (pbuf);
	  return ENOMEM;
	}
      else
	pbuf = tbuf;
      memcpy (pbuf + offset, buf, nread);
      offset += nread;
    } while (nread > 0);

  /* get an empty message struct */
  status = message_create (&msg, mum);
  if (status != 0)
    {
      free (pbuf);
      return status;
    }

  /* set the header */
  status = header_create (&header, pbuf, offset, mum);
  if (status != 0)
    {
      free (pbuf);
      message_destroy (&msg, mum);
      return status;
    }
  free (pbuf);
  message_set_header (msg, header, mum);

  /* prepare the body */
  status = body_create (&body, mum);
  if (status != 0)
    {
      message_destroy (&msg, mum);
      return status;
    }
  message_set_body (msg, body, mum);

  status = stream_create (&stream, MU_STREAM_READ, mum);
  if (status != 0)
    {
      message_destroy (&msg, mum);
      return status;
    }
  stream_set_read (stream, mailbox_unix_readstream, mum);
  stream_set_fd (stream, mailbox_unix_getfd, mum);
  stream_get_flags (mbox->stream, &flags);
  stream_set_flags (stream, flags, mum);
  body_set_stream (body, stream, mum);
  body_set_size (body, mailbox_unix_body_size, mum);
  body_set_lines (body, mailbox_unix_body_lines, mum);

  /* set the attribute */
  attribute_create (&(mum->new_attr));
  mum->new_attr->flag = mum->old_attr->flag;
  status = message_set_attribute (msg, mum->new_attr, mum);
  if (status != 0)
    {
      message_destroy (&msg, mum);
      return status;
    }

  /* set the envelope */
  message_set_from (msg, mailbox_unix_msg_from, mum);
  message_set_received (msg, mailbox_unix_msg_received, mum);

  /* attach the message to the mailbox unix data */
  mum->message = msg;

  if (pmsg)
    *pmsg = msg;
  return 0;
}

static int
mailbox_unix_append_message (mailbox_t mbox, message_t msg)
{
  mailbox_unix_data_t mud;
  if (mbox == NULL || msg == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  mailbox_unix_lock (mbox, MU_LOCKER_WRLOCK);
  {
    off_t size;
    char buffer[BUFSIZ];
    size_t nread = 0;
    off_t off = 0;
    stream_t is;
    header_t hdr;
    int status;
    size_t n = 0;
    char nl = '\n';

    /* move to the end of the file, not necesary if _APPEND mode */
    status = stream_size (mbox->stream, &size);
    if (status != 0)
      {
	mailbox_unix_unlock (mbox);
	return status;
      }
    /* header */
    message_get_header (msg, &hdr);
    /* generate a "From " separator */
    {
      char from[128];
      char date[128];
      char *s;
      size_t f = 0, d = 0;
      *date = *from = '\0';
      message_from (msg, from, sizeof (from), &f);
      s = memchr (from, '\n', f);
      if (s)
	{
	  *s = '\0';
	  f--;
	}
      message_received (msg, date, sizeof (date), &d);
      s = memchr (date, '\n', d);
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

    header_get_stream (hdr, &is);
    do {
      status = stream_read (is, buffer, sizeof (buffer), off, &nread);
      if (status != 0)
	return status;
      if (nread == 0)
	break;
      stream_write (mbox->stream, buffer, nread, size, &n);
      off += nread;
      size += n;
    } while (nread > 0);

    *buffer = '\0';
    /* separator */
    /*fputc ('\n', mud->file);*/

    /* body */
    message_get_stream (msg, &is);
    do {
      stream_read (is, buffer, sizeof (buffer), off, &nread);
      stream_write (mbox->stream, buffer, nread, size, &n);
      off += nread;
      size += n;
    } while (nread > 0);
    stream_write (mbox->stream, &nl, 1, size, &n);
  }
  stream_flush (mbox->stream);
  mailbox_unix_unlock (mbox);
  return 0;
}

static int
mailbox_unix_size (mailbox_t mbox, off_t *psize)
{
  off_t size;
  int status;

  if (mbox == NULL)
    return EINVAL;

  /* maybe was not open yet ?? */
  status  = stream_size (mbox->stream, &size);
  if (status != 0)
    return status;
  if (psize)
    *psize = size;
  return 0;
}

static int
mailbox_unix_messages_count (mailbox_t mbox, size_t *pcount)
{
  mailbox_unix_data_t mud;
  if (mbox == NULL || (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;

  if (! mailbox_unix_is_updated (mbox))
    return mailbox_unix_scan0 (mbox,  1, pcount, 1);

  if (pcount)
    *pcount = mud->messages_count;

  return 0;
}

/* locking */
static int
mailbox_unix_lock (mailbox_t mbox, int flag)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_lock (locker, flag);
    }
  return 0;
}

static int
mailbox_unix_touchlock (mailbox_t mbox)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_touchlock (locker);
    }
  return 0;
}

static int
mailbox_unix_unlock (mailbox_t mbox)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_unlock (locker);
    }
  return 0;
}

static int
mailbox_unix_ilock (mailbox_t mbox, int flag)
{
#ifdef HAVE_PTHREAD_H
  (void)flag; /* we should use rwlocks for more concurency */
  if (mbox == NULL)
    return EINVAL;
  return pthread_mutex_lock (&(mbox->mutex));
#else
  (void)mbox; (void)flag;
  return 0;
#endif
}

static int
mailbox_unix_iunlock (mailbox_t mbox)
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
