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
 * and ends. old_attr is the one that is part of the "Status:" message.
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
  FILE *file;

  /* if we have a message attach to it */
  message_t message;

} *mailbox_unix_message_t;

/*
 * umessages is an array of pointers that contains umessages_count
 * of mailbox_unix_message_t*; umessages[umessages_count].
 * We do it this because realloc() can move everything to
 * a new memory region and invalidating all the pointers someone
 * has on the message.  Thanks to <Dave Inglis> for pointing this out.
 * messages_count is the count number of messages parsed so far.
 */
typedef struct _mailbox_unix_data
{
  mailbox_unix_message_t *umessages;
  size_t umessages_count;
  size_t messages_count;
  FILE *file;
  char *dirname;
  char *basename;
  int flags;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  time_t mtime;
  off_t size;

} *mailbox_unix_data_t;

static int mailbox_unix_open (mailbox_t mbox, int flag);
static int mailbox_unix_close (mailbox_t mbox);

static int mailbox_unix_get_message (mailbox_t, size_t msgno, message_t *msg);
static int mailbox_unix_append_message (mailbox_t, message_t msg);
static int mailbox_unix_messages_count (mailbox_t, size_t *msgno);
static int mailbox_unix_expunge (mailbox_t);
static int mailbox_unix_num_deleted (mailbox_t, size_t *);

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

  /* save the basename and dirname */
  /* FIXME: We may have to support imap "SELECT"
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
  pthread_mutex_init (&(mud->mutex), NULL);
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
	      message_destroy (&(mum->message), mum);
	      attribute_destroy (&(mum->old_attr));
	      attribute_destroy (&(mum->new_attr));
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
      free (*pmbox);
      *pmbox = NULL;
    }
}

/* Open the file. */
static int
mailbox_unix_open (mailbox_t mbox, int flags)
{
  mailbox_unix_data_t mud;
  int fd = -1;
  int flg = 0;
  const char *mode;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  if (flags & MU_MAILBOX_WRONLY)
    flg = O_WRONLY;
  else if (flags & MU_MAILBOX_RDWR)
    flg = O_RDWR;
  else /* default */
    flg = O_RDONLY;

  /* FIXME: only if the file is open Write */
  if (flags & MU_MAILBOX_APPEND)
    flg |= O_APPEND;

  /* but local folders should not block since it is local disk ??? */
  if (flags & MU_MAILBOX_NONBLOCK)
    flg |= O_NONBLOCK;

  /* Authentication prologues */
  if (mbox->auth)
    auth_prologue (mbox->auth);

  /* handle CREAT with care, not to follow symlinks */
  if (flags & MU_MAILBOX_CREAT)
    {
      /* Grab the lock for any race conditions */
      mailbox_unix_lock (mbox, MU_LOCKER_WRLOCK);
      {
	/* first see if the file already exists */
	fd = open(mbox->name, flg);
	if (fd == -1)
	  {
	    /* oops bail out */
	    if (errno != ENOENT)
	      return errno;
	    /* Race condition here when creating the file ?? */
	    fd = open(mbox->name, flg|O_CREAT|O_EXCL, 0600);
	    if (fd < 0)
	      return errno;
	  }
      }
      mailbox_unix_unlock (mbox);
    }
  else
    {
      fd = open (mbox->name, flg);
      if (fd < 0)
	return errno;
    }

  /* FIXME: FIXME: FIXME: FIXME:
   * before going any further I should check for symlinks etc
   * and refuse to go any furter, please fix asap.
   */

  /* Authentication */
  if (mbox->auth)
    {
      char *user = NULL;
      char *passwd = NULL;
      int status = auth_authenticate (mbox->auth, &user, &passwd);
      free (user);
      free (passwd);
      if (status != 0)
	return status;
    }
  else
    {
      /*
       * Set the owner ship, but should we return the errno
       */
      gid_t g ; uid_t u; mode_t m;
      g = u = -1; m = 0600;
      /* FIXME: what to do when they failed */
      (void)fchown (fd, u, g);
      (void)fchmod (fd, m);
    }

  /* Authentication epilogues */
  if (mbox->auth)
    auth_epilogue (mbox->auth);

  /* we use FILE * object */
  if (flags & MU_MAILBOX_APPEND)
    mode = "a";
  else if (flags & MU_MAILBOX_RDWR || flags & MU_MAILBOX_WRONLY)
    mode = "r+";
  else /* default readonly*/
    mode = "r";

  /* clean up, make sure there was nothing before  */
  mailbox_unix_close (mbox);

  mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK);
  {
    mud->file = fdopen (fd, mode);
    if (mud->file == NULL)
      {
	/* what's up ?? */
	int err = errno;
	mailbox_unix_iunlock (mbox);
	return err;
      }
    /* Is this check necessary ?? */
    /* Check to make sure this is indeed a Unix Mail format */
    flockfile (mud->file);
    {
      char buf [6];
      if (fgets (buf, sizeof (buf), mud->file) == NULL)
	{
	  int status = errno;
	  if (feof (mud->file))
	    clearerr (mud->file); /* the file maybe empty */
	  else if (ferror (mud->file))
	    {
	      mailbox_unix_iunlock (mbox);
	      return status;
	    }
	}
      else
	{
	  if (strncmp ("From ", buf, 5) != 0)
	    {
	      /* not a Unix mbox */
	      fclose (mud->file);
	      mailbox_unix_iunlock (mbox);
	      return EIO;
	    }
	}
      rewind (mud->file);
    }
    funlockfile (mud->file);
    mud->flags = flags;
  }
  mailbox_unix_iunlock (mbox);
  return 0;
}

static int
mailbox_unix_close (mailbox_t mbox)
{
  mailbox_unix_data_t mud;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK);
  {
    if (mud->file)
      fclose (mud->file);
    mud->file = NULL;
    /* make sure we do not hold any lock for that file */
    mailbox_unix_unlock (mbox);
  }
  mailbox_unix_iunlock (mbox);
  return 0;
}

/* Mailbox Parsing */
#include "mbx_unixscan.c"

/* FIXME:  How to handle a shrink ? meaning, the &^$^@%#@^& user
 * start two browsers and delete files in one.  My views
 * is that we should scream bloody murder and hunt them with
 * a machette.
 */
static int
mailbox_unix_is_updated (mailbox_t mbox)
{
  mailbox_unix_data_t mud;
  struct stat st;
      if ((mud = (mailbox_unix_data_t)mbox->data) == NULL ||
	  fstat (fileno (mud->file), &st) < 0)
    return 0;
  return (mud->mtime == st.st_mtime);
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
mailbox_unix_tmpfile (mailbox_t mbox, char *tmpmbox)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t)mbox->data;
  int fd;
  FILE *fp;

#ifndef P_tmpdir
# define P_tmpdir "/tmp"
#endif

  tmpmbox = calloc (1, strlen (P_tmpdir) + strlen ("MBOX_") +
		   strlen (mud->basename) + 1);
  if (tmpmbox == NULL)
    return NULL;
  sprintf (tmpmbox, "%s/%s%s", P_tmpdir, "MBOX_", mud->basename);

  /* FIXME:  I don't think this is the righ approach
   * Creating an anonymous file would be better ?
   * no trace left behind.
   */
  /* Create the file.  It must not exist.  If it does exist, fail. */
  fd = open(tmpmbox, O_RDWR|O_CREAT|O_EXCL, 0600);
  if (fd < 0)
    {
      fprintf(stderr,"Can't create %s\n", tmpmbox);
      fprintf(stderr,"delete file <%s>, Please\n", tmpmbox);
      return NULL;
    }
  fp = fdopen(fd, "w+");
  if (fp == 0)
    {
      close(fd);
      free (tmpmbox);
      tmpmbox = NULL;
    }

  /* really I should just remove the file here */
  /* remove(tmpmbox); */
  return fp;
}

static int
mailbox_unix_expunge (mailbox_t mbox)
{
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;
  int status = 0;
  int oflags;
  sigset_t sigset;
  FILE *tempfile;
  size_t nread;
  size_t i, j, first;
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

  tempfile = mailbox_unix_tmpfile (mbox, tmpmbox);
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

  /* FIXME: send notification to all the listeners */

  /* Critical section, we can not allowed signal here */
  /* FIXME: If NONBLOCKING is set we should unset reset to
   * later, we can not afford to luxury here
   */
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGTERM);
  sigaddset (&sigset, SIGHUP);
  sigaddset (&sigset, SIGTSTP);
  sigaddset (&sigset, SIGINT);
  sigaddset (&sigset, SIGWINCH);
  sigprocmask (SIG_BLOCK, &sigset, 0);

  mailbox_unix_ilock (mbox, MU_LOCKER_RDLOCK);
  flockfile (mud->file);

  /*
   * We can not be NONBLOCKING here.
   * It would irresponsable.
   */
  if ((oflags = fcntl (fileno (mud->file), F_GETFL, 0)) < 0)
    {
      status = errno;
      goto bailout;
    }
  fcntl (fileno (mud->file), F_SETFL, oflags & ~O_NONBLOCK);

  /* Do we have a consistent view of the mailbox */
  /* FIXME: this is not enough we can do better
   * - by checking the file size and scream bloody murder
   * if it has shrink.
   * - if its bigger we should be able to handle it
   * before the ftruncate() by copying back the new messages.
   */
  if (! mailbox_unix_is_updated (mbox))
    {
      status = EAGAIN;
      goto bailout;
    }

  rewind (mud->file);

  /* Go to the first mail with an attribute change */
  for (j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (! attribute_is_equal (mum->old_attr, mum->new_attr))
	break;
    }

  /* did something change ? */
  if ((j + 1) == mud->messages_count)
    {
      /* nothing change, bail out */
      status = 0;
      goto bailout;
    }

  /* set the marker position */
  total = marker = mud->umessages[j]->header_from;

  /* copy to tempfile emails not mark changed */
  for (first = 1, i = j; i < mud->messages_count; i++)
    {
      size_t len;
      off_t current;
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
      /* copy the header */
      if (fseek (mud->file, mum->header_from, SEEK_SET) == -1)
	{
	  status = errno;
	  goto bailout;
	}
      /* attribute change ? */
      if (! attribute_is_equal (mum->old_attr, mum->new_attr) &&
	  mum->header_status > mum->header_from)
	{
	  len = mum->header_status - mum->header_from;
	  current = mum->header_status_end;
	  while (len > 0)
	    {
	      nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	      if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
		  || fwrite(buffer, sizeof(*buffer), nread, tempfile) != nread)
		{
		  status = errno;
		  goto bailout;
		}
	      len -= nread;
	      total += nread;
	    }
	  /* put the new attributes */
	  {
	    attribute_t attr = mum->new_attr;
	    fputs ("Status: ", tempfile);
	    total += 8;
	    if (attribute_is_seen (attr))
	      {
		fputc ('R', tempfile);
		total++;
	      }
	    if (attribute_is_answered (attr))
	      {
		fputc ('A', tempfile);
		total++;
	      }
	    if (attribute_is_flagged (attr))
	      {
		fputc ('F', tempfile);
		total++;
	      }
	    if (attribute_is_read (attr))
	      {
		fputc ('O', tempfile);
		total++;
	      }
	    fputc ('\n', tempfile);
	    total++;
	  }
	  /* skip the status field */
	  if (fseek (mud->file, current, SEEK_SET) == -1)
	    {
	      status = errno;
	      goto bailout;
	    }
	}
      else /* attribute did not change */
	{
	  current = mum->header_from;
	}
      len = mum->body - current;
      while (len > 0)
	{
	  nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	  if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
	      || fwrite(buffer, sizeof(*buffer), nread, tempfile) != nread)
	    {
	      status = errno;
	      goto bailout;
	    }
	  len -= nread;
	  total += nread;
	}

      /* Separate the header from body */
      /*fputc ('\n', tempfile); */
      /*total++;*/

      /* copy the body */
      if (fseek (mud->file, mum->body, SEEK_SET) < 0)
	{
	  status = errno;
	  goto bailout;
	}
      len = mum->body_end - mum->body;
      while (len > 0)
	{
	  nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	  if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
	      || fwrite(buffer, sizeof(*buffer), nread, tempfile) != nread)
	    {
	      status = errno;
	      goto bailout;
	    }
	  len -= nread;
	  total += nread;
	}
    }

  /*
   * Caution:
   * before truncating the file see if we've receiving new mail
   * Some program may not respect the lock.
   */
  {
    struct stat st;
    if (fstat (fileno (mud->file), &st) != 0)
      {
	status = errno;
	goto bailout;
      }
    if (st.st_size > mud->size)
      {
	size_t len = st.st_size - mud->size;
	if (fseek (mud->file, mud->size, SEEK_SET) < 0)
	  {
	    status = errno;
	    goto bailout;
	  }
	while (len > 0)
	  {
	    nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	    if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
		|| fwrite(buffer, sizeof(*buffer), nread, tempfile) != nread)
	      {
		status = errno;
		goto bailout;
	      }
	    len -= nread;
	    total += nread;
	  }
      }
  }
  /* Seek and rewrite it */
  if (total <= 0 || fseek (mud->file, marker, SEEK_SET) < 0)
    {
      status = errno;
      goto bailout;
    }
  rewind (tempfile);

  errno = 0;
  while ((nread = fread (buffer, sizeof (*buffer),
			 sizeof (buffer), tempfile)) != 0)
    {
      if (fwrite (buffer, sizeof (*buffer), nread, mud->file) != nread)
	{
	  if (errno == EAGAIN || errno == EINTR)
	    {
	      errno = 0;
	      continue;
	    }
	  status = errno;
	  goto bailout;
	}
    }

  /* how can I handle error here ?? */
  clearerr (tempfile);
  clearerr (mud->file);
  fflush (mud->file);

  /* truncation */
  if (ftruncate (fileno (mud->file), total) < 0)
    goto bailout;

  /* FIXME: update the num of all the messages */

  /* Don't remove the tmp mbox in case of errors */
  remove (tmpmbox);

bailout:
  free (tmpmbox);
  /* Release the locks */
  if (oflags > 0)
    fcntl (fileno (mud->file), F_SETFL, oflags);
  mailbox_unix_unlock (mbox);
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);
  fclose (tempfile);
  sigprocmask (SIG_UNBLOCK, &sigset, 0);
  return status;
}

static int
mailbox_unix_getfd (stream_t is, int *pfd)
{
  mailbox_unix_message_t mum;

  if (is == NULL || (mum = is->owner) == NULL)
    return EINVAL;
  if (pfd)
    *pfd = fileno (mum->file);
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

  flockfile (mum->file);
  {
    off_t ln = mum->body_end - (mum->body + off);
    if (ln > 0)
      {
	nread = ((size_t)ln < buflen) ? ln : buflen;
	/* position the file pointer and the buffer */
	if (fseek (mum->file, mum->body + off, SEEK_SET) < 0 ||
	    fread (buffer, sizeof (*buffer), nread, mum->file) != nread)
	  {
	    funlockfile (mum->file);
	    return errno;
	  }
      }
  }
  funlockfile (mum->file);

  if (pnread)
    *pnread = nread;
  return 0;
}

static int
mailbox_unix_get_header (mailbox_unix_message_t mum, char *buffer,
			 size_t len, off_t off, ssize_t *pnread)
{
  size_t nread = 0;
  FILE *file = mum->file;

  flockfile (file);
  {
    off_t ln = mum->body - (mum->header_from_end + off);
    if (ln > 0)
      {
	nread = ((size_t)ln < len) ? ln : len;
	/* position the file pointer and the buffer */
	if (fseek (file, mum->header_from_end + off, SEEK_SET) < 0 ||
	    fread (buffer, sizeof (*buffer), nread, file) != nread)
	  {
	    funlockfile (file);
	    return errno;
	  }
      }
  }
  funlockfile (file);

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
  char buffer[512];

  if (mum == NULL)
    return EINVAL;

  flockfile (mum->file);
  {
    if (fseek (mum->file, mum->header_from, SEEK_SET) < 0 ||
	fgets (buffer, sizeof (buffer), mum->file) == NULL)
      {
	if (pnwrite)
	  *pnwrite = 0;
	if (buf)
	  *buf = '\0';
	return errno;
      }
  }
  funlockfile (mum->file);

  if (strlen (buffer) > 5)
    {
      char *s;
      s = strchr (buffer + 5, ' ');
      if (s)
	{
	  if (buf && len > 0)
	    {
	      strncpy (buf, s + 1, len);
	      buffer [len - 1] = '\0';
	    }
	  if (pnwrite)
	    *pnwrite = strlen (s);
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
  char buffer[512];

  if (mum == NULL)
    return EINVAL;

  flockfile (mum->file);
  {
    if (fseek (mum->file, mum->header_from, SEEK_SET) < 0 ||
	fgets (buffer, sizeof (buffer), mum->file) == NULL)
      {
	if (pnwrite)
	  *pnwrite = 0;
	if (buf)
	  *buf = '\0';
	return errno;
      }
  }
  funlockfile (mum->file);

  if (strlen (buffer) > 5)
    {
      char *s;
      s = strchr (buffer + 5, ' ');
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
  body_set_stream (body, stream, mum);
  body_set_size (body, mailbox_unix_body_size, mum);
  body_set_lines (body, mailbox_unix_body_lines, mum);

  /* set the attribute */
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

  /* simple check on the flag */
  if (! ((mud->flags & MU_MAILBOX_APPEND) ||
	 (mud->flags & MU_MAILBOX_RDWR) ||
	 (mud->flags & MU_MAILBOX_WRONLY)))
    return EACCES;

  mailbox_unix_lock (mbox, MU_LOCKER_WRLOCK);
  flockfile (mud->file);
  {
    struct stat st;
    int fd;
    char buffer[BUFSIZ];
    size_t nread = 0;
    off_t off = 0;
    stream_t is;
    header_t hdr;
    int status;

    /* move to the end of the file, not necesary if _APPEND mode */
    fd = fileno (mud->file);
    if (fstat (fd, &st) != 0)
      {
	funlockfile (mud->file);
	mailbox_unix_unlock (mbox);
	return errno;
      }
    if (fseek(mud->file, st.st_size, SEEK_SET) != 0)
      {
	funlockfile (mud->file);
	mailbox_unix_unlock (mbox);
	return errno;
      }

    /* header */
    if (st.st_size != 0)
      fputc ('\n', mud->file);

    message_get_header (msg, &hdr);
    /* generate a "From " separator */
    {
      char from[128];
      char date[128];
      char *s;
      size_t n = 0;
      *date = *from = '\0';
      message_from (msg, from, sizeof (from), &n);
      s = memchr (from, '\n', n);
      if (s) *s = '\0'; n = 0;
      message_received (msg, date, sizeof (date), &n);
      s = memchr (date, '\n', n);
      if (s) *s = '\0';
      fprintf (mud->file, "From %s %s\n", from, date);
    }

    header_get_stream (hdr, &is);
    do {
      status = stream_read (is, buffer, sizeof (buffer), off, &nread);
      if (status != 0)
	return status;
      if (nread == 0)
	break;
      fwrite (buffer, sizeof (*buffer), nread, mud->file);
      off += nread;
    } while (nread > 0);

    *buffer = '\0';
    /* separator */
    /*fputc ('\n', mud->file);*/

    /* body */
    message_get_stream (msg, &is);
    do {
      stream_read (is, buffer, sizeof (buffer), off, &nread);
      fwrite (buffer, sizeof (*buffer), nread, mud->file);
      off += nread;
    } while (nread > 0);
  }
  fflush(mud->file);
  funlockfile (mud->file);
  mailbox_unix_unlock (mbox);
  return 0;
}

static int
mailbox_unix_size (mailbox_t mbox, off_t *psize)
{
  mailbox_unix_data_t mud;
  struct stat st;
  int fd;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* maybe was not open yet ?? */
  if (mud->file == NULL)
      return EIO;
  fd = fileno (mud->file);
  /* oops !! */
  if (fstat (fd, &st) != 0)
    return errno;
  if (psize)
    *psize = st.st_size;
  return 0;
}

static int
mailbox_unix_messages_count (mailbox_t mbox, size_t *pcount)
{
  mailbox_unix_data_t mud;
  if (mbox == NULL || (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;

  if (! mailbox_unix_is_updated (mbox))
    return mailbox_unix_scan (mbox,  1, pcount);

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
  mailbox_unix_data_t mud;
  (void)flag; /* we should use rwlocks for more concurency */
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;
  return pthread_mutex_lock (&(mud->mutex));
#else
  (void)mbox; (void)flag;
  return 0;
#endif
}

static int
mailbox_unix_iunlock (mailbox_t mbox)
{
#ifdef HAVE_PTHREAD_H
  mailbox_unix_data_t mud;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;
  return pthread_mutex_unlock (&(mud->mutex));
#else
  (void)mbox;
  return 0;
#endif
}
