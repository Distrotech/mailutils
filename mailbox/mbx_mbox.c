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

#include <mailutils/message.h>
#include <mailutils/stream.h>
#include <mailutils/body.h>
#include <mailutils/header.h>
#include <mailutils/attribute.h>
#include <registrar0.h>
#include <mailbox0.h>


#define ATTRIBUTE_IS_DELETED(flag)        (flag & MU_ATTRIBUTE_DELETED)
#define ATTRIBUTE_IS_EQUAL(flag1, flag2)  (flag1 == flag2)

static int mbox_init (mailbox_t mailbox);

/* Register variables.  */
static struct mailbox_entry _mbox_entry =
{
  url_mbox_init, mbox_init
};
mailbox_entry_t mbox_entry = &_mbox_entry;

static struct _record _mbox_record =
{
  MU_MBOX_SCHEME,
  &_mbox_entry, /* Mailbox entry.  */
  NULL, /* Mailer entry.  */
  NULL, /* Folder entry.  */
  0, /* Not malloc()ed.  */
  NULL, /* No need for an owner.  */
  NULL, /* is_scheme method.  */
  NULL, /* get_mailbox method.  */
  NULL, /* get_mailer method.  */
  NULL /* get_folder method.  */
};
record_t mbox_record = &_mbox_record;

static void mbox_destroy (mailbox_t);

struct _mbox_message;
struct _mbox_data;

typedef struct _mbox_data* mbox_data_t;
typedef struct _mbox_message* mbox_message_t;

/* Keep the position of where the header and body starts and ends.
   old_flags is the "Status:" message.  */
struct _mbox_message
{
  /* Offset of the parts of the messages in the mailbox.  */
  off_t header_from;
  off_t header_from_end;
  /* Little hack to make things easier when updating the attribute.  */
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
  mbox_data_t mud; /* Back pointer.  */
};

/* The umessages is an array of pointers that contains umessages_count of
   mbox_message_t*; umessages[umessages_count].  We do it this way because
   realloc() can move everything to a new memory region and invalidate all
   the pointers.  Thanks to <Dave Inglis> for pointing this out.  The
   messages_count is the count number of messages parsed so far.  */
struct _mbox_data
{
  mbox_message_t *umessages; /* Array.  */
  size_t umessages_count; /* How big is the umessages[].  */
  size_t messages_count; /* How many valid entry in umessages[].  */
  stream_t stream;
  off_t size;
  char *name;

  /* The variables below are use to hold the state when appending messages.  */
  enum mbox_state
  {
    MBOX_NO_STATE=0, MBOX_STATE_FROM, MBOX_STATE_DATE, MBOX_STATE_APPEND
  } state ;
  char *from;
  char *date;
  off_t off;
  mailbox_t mailbox; /* Back pointer. */
};

/* Mailbox concrete implementation.  */
static int mbox_open (mailbox_t, int);
static int mbox_close (mailbox_t);
static int mbox_get_message (mailbox_t, size_t, message_t *);
static int mbox_append_message (mailbox_t, message_t);
static int mbox_messages_count (mailbox_t, size_t *);
static int mbox_expunge (mailbox_t);
static int mbox_scan (mailbox_t, size_t, size_t *);
static int mbox_is_updated (mailbox_t);
static int mbox_size (mailbox_t, off_t *);


/* private stuff */
static int mbox_scan0 (mailbox_t, size_t, size_t *, int);
static int mbox_get_header_read (stream_t, char *, size_t, off_t, size_t *);
static int mbox_get_hdr_fd (stream_t, int *);
static int mbox_get_body_fd (stream_t, int *);
static int mbox_get_fd (mbox_message_t, int *);
static int mbox_get_attr_flags (attribute_t, int *);
static int mbox_set_attr_flags (attribute_t, int);
static int mbox_unset_attr_flags (attribute_t, int);
static int mbox_readstream (stream_t, char *, size_t, off_t, size_t *);
static int mbox_header_size (header_t, size_t *);
static int mbox_header_lines (header_t, size_t *);
static int mbox_body_size (body_t, size_t *);
static int mbox_body_lines (body_t, size_t *);
static int mbox_msg_from (message_t, char *, size_t, size_t *);
static int mbox_msg_received (message_t, char *, size_t, size_t *);
static int mbox_lock (mailbox_t, int);
static int mbox_touchlock (mailbox_t);
static int mbox_unlock (mailbox_t);

/* We allocate the mbox_data_t struct, but don't do any parsing on the name or
   even test for existence.  However we do strip any leading "mbox:" part of
   the name, this is suppose to be the protocol/scheme name.  */
static int
mbox_init (mailbox_t mailbox)
{
  mbox_data_t mud;
  size_t name_len;

  if (mailbox == NULL)
    return EINVAL;

  /* Allocate specific mbox data.  */
  mud = mailbox->data = calloc (1, sizeof (*mud));
  if (mailbox->data == NULL)
    return ENOMEM;

  /* Back pointer.  */
  mud->mailbox = mailbox;

  /* Copy the name.
     We do not do any further interpretation after the scheme "mbox:"
     Because for example on distributed system like QnX4 a file name is
     //390/etc/passwd.  So the best approach is to let the OS handle it
     for example if we receive: "mbox:///var/mail/alain" the mailbox name
     will be "///var/mail/alain", we let open() do the right thing.
     So it will let things like this "mbox://390/var/mail/alain" where
     "//390/var/mail/alain" _is_ the filename, pass correctely.  */
  url_get_path (mailbox->url, NULL, 0, &name_len);
  mud->name = calloc (name_len + 1, sizeof (char));
  if (mud->name == NULL)
    {
      mbox_destroy (mailbox);
      return ENOMEM;
    }
  url_get_path (mailbox->url, mud->name, name_len + 1, NULL);

  mud->state = MBOX_NO_STATE;

  /* Overloading the default.  */
  mailbox->_init = mbox_init;
  mailbox->_destroy = mbox_destroy;

  mailbox->_open = mbox_open;
  mailbox->_close = mbox_close;

  /* Messages.  */
  mailbox->_get_message = mbox_get_message;
  mailbox->_append_message = mbox_append_message;
  mailbox->_messages_count = mbox_messages_count;
  mailbox->_expunge = mbox_expunge;

  mailbox->_scan = mbox_scan;
  mailbox->_is_updated = mbox_is_updated;

  mailbox->_size = mbox_size;

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_init(%s)\n", mud->name);
  return 0; /* okdoke */
}

/* Free all ressources associated with Unix mailbox.  */
static void
mbox_destroy (mailbox_t mailbox)
{
  mbox_close (mailbox);
  if (mailbox->data)
    {
      size_t i;
      mbox_data_t mud = mailbox->data;
      MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE,
		      "mbox_destroy (%s)\n", mud->name);
      mailbox_wrlock (mailbox);
      for (i = 0; i < mud->umessages_count; i++)
	{
	  mbox_message_t mum = mud->umessages[i];
	  if (mum)
	    {
	      message_destroy (&(mum->message), mum);
	      free (mum);
	    }
	}
      if (mud->umessages)
	free (mud->umessages);
      if (mud->name)
	free (mud->name);
      free (mud);
      mailbox->data = NULL;
      mailbox_unlock (mailbox);
    }
}

/* Open the file.  */
static int
mbox_open (mailbox_t mailbox, int flags)
{
  mbox_data_t mud = mailbox->data;
  int status = 0;

  if (mud == NULL)
    return EINVAL;

  mailbox->flags = flags | MU_STREAM_FILE;
  mailbox_rdlock (mailbox);

  /* Get a stream.  */
  if (mailbox->stream == NULL)
    {
      /* FIXME: for small mbox we should try to mmap ().  */

      status = (flags & MU_STREAM_CREAT) || (mailbox->flags & MU_STREAM_APPEND);
      if (status == 0)
	status = mapfile_stream_create (&(mailbox->stream));
      if (status != 0)
	{
	  status = file_stream_create (&(mailbox->stream));
	  if (status != 0)
	    {
	      mailbox_unlock (mailbox);
	      return status;
	    }
	}
      status = stream_open (mailbox->stream, mud->name, 0, mailbox->flags);
      if (status != 0)
	{
	  mailbox_unlock (mailbox);
	  return status;
	}
    }
  else
    {
      status = stream_open (mailbox->stream, mud->name, 0, mailbox->flags);
      if (status != 0)
	{
	  mailbox_unlock (mailbox);
	  return status;
	}
    }

  MAILBOX_DEBUG2 (mailbox, MU_DEBUG_TRACE, "mbox_open(%s, %d)\n",
		  mud->name, mailbox->flags);

  /* Give an appopriate way to lock.  */
  if (mailbox->locker == NULL)
    locker_create (&(mailbox->locker), mud->name, strlen (mud->name),
		   MU_LOCKER_PID | MU_LOCKER_FCNTL);
  mailbox_unlock (mailbox);
  return 0;
}

static int
mbox_close (mailbox_t mailbox)
{
  mbox_data_t mud = mailbox->data;
  size_t i;

  if (mud == NULL)
    return EINVAL;

  /* Make sure we do not hold any lock for that file.  */
  mbox_unlock (mailbox);
  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE,  "mbox_close(%s)\n", mud->name);

  mailbox_wrlock (mailbox);
  /* Before closing we need to remove all the messages
     - to reclaim the memory
     - to prepare for another scan.  */
  for (i = 0; i < mud->umessages_count; i++)
    {
      mbox_message_t mum = mud->umessages[i];
      /* Destroy the attach messages.  */
      if (mum)
	{
	  message_destroy (&(mum->message), mum);
	  free (mum);
	}
    }
  if (mud->umessages)
    free (mud->umessages);
  mud->umessages = NULL;
  mud->messages_count = mud->umessages_count = 0;
  mud->size = 0;
  mailbox_unlock (mailbox);
  return stream_close (mailbox->stream);
}

/* Mailbox Parsing.  */
#include "mbx_mboxscan.c"

static int
mbox_scan (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  mbox_data_t mud = mailbox->data;
  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_scan(%s)\n", mud->name);
  return mbox_scan0 (mailbox, msgno, pcount, 1);
}


/* FIXME:  How to handle a shrink ? meaning, the &^$^@%#@^& user start two
   browsers and delete files in one.  My views is that we should scream
   bloody murder and hunt them with a machette. But for now just play dumb,
   but maybe the best approach is to pack our things and leave .i.e exit().  */
static int
mbox_is_updated (mailbox_t mailbox)
{
  off_t size;
  mbox_data_t mud = mailbox->data;
  int status;

  if (mud == NULL)
    return EINVAL;
  mailbox_rdlock (mailbox);
  if (stream_size (mailbox->stream, &size) != 0)
    {
      mailbox_unlock (mailbox);
      return 0;
    }
  status = (mud->size == size);
  mailbox_unlock (mailbox);
  return status;
}

/* FIXME: the use of tmpfile() on some system can lead to race condition, We
   should use a safer approach.  */
static FILE *
mbox_tmpfile (mailbox_t mailbox, char **pbox)
{
  const char *tmpdir;
  int fd;
  FILE *fp;
  const char *basename;
  mbox_data_t mud = mailbox->data;

  /*  P_tmpdir should be define in <stdio.h>.  */
#ifndef P_tmpdir
#  define P_tmpdir "/tmp"
#endif

  tmpdir =  getenv ("TEMPDIR") ? getenv ("TEMPDIR") : P_tmpdir;
  basename = strrchr (mud->name, '/');
  if (basename)
    basename++;
  else
    basename = mud->name;
  *pbox = calloc (strlen (tmpdir) + strlen ("MBOX_") +
		  strlen (basename) + 1, sizeof (char));
  if (*pbox == NULL)
    return NULL;
  sprintf (*pbox, "%s/%s%s", tmpdir, "MBOX_", basename);

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

/* For the expunge bits  we took a very cautionnary approach, meaning
   we create temporary file in the tmpdir copy all the file not mark deleted,
   and skip the deleted ones, truncate the real mailbox to the desired size
   and overwrite with the tmp mailbox.  The approach to do everyting
   in core is tempting but require to much memory, it is not rare now
   a day to have 30 Megs mailbox, also there is danger for filesystems
   with quotas, or some program may not respect the advisory lock and
   decide to append a new message while your expunging etc ...
   The real downside to the approach we take is that when things go wrong
   the temporary file bay be left in /tmp, which is not all that bad
   because at least have something to recuparate when failure.  */
static int
mbox_expunge (mailbox_t mailbox)
{
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  int status = 0;
  sigset_t signalset;
  FILE *tempfile;
  size_t nread;
  size_t i, j, dirty, first;
  off_t marker = 0;
  off_t total = 0;
  char buffer [BUFSIZ];
  char *tmpmbox = NULL;

  if (mud == NULL)
    return EINVAL;

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_expunge (%s)\n", mud->name);

  /* Noop.  */
  if (mud->messages_count == 0)
    return 0;

  /* Do we have a consistent view of the mailbox.  */
  if (! mbox_is_updated (mailbox))
    {
      fprintf (stderr, "mailbox is not updated, try again.\n");
      return  EAGAIN;
    }

  mailbox_wrlock (mailbox);

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
    {
      mailbox_unlock (mailbox);
      return 0; /* Nothing change, bail out.  */
    }


  /* This is redundant, we go to the loop again.  But it's more secure here
     since we don't want to be disturb when expunging.  */
  for (j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && mum->new_flags && ATTRIBUTE_IS_DELETED (mum->new_flags))
	message_destroy (&(mum->message), mum);
    }

  /* Create a tempory file.  */
  tempfile = mbox_tmpfile (mailbox, &tmpmbox);
  if (tempfile == NULL)
    {
      fprintf (stderr, "Failed to create temporary file when expunging.\n");
      free (tmpmbox);
      mailbox_unlock (mailbox);
      return errno;
    }

  /* Get the lock.  */
  if (mbox_lock (mailbox, MU_LOCKER_WRLOCK) < 0)
    {
      fclose (tempfile);
      remove (tmpmbox);
      free (tmpmbox);
      fprintf (stderr, "Failed to grab the lock\n");
      mailbox_unlock (mailbox);
      return ENOLCK;
    }

  /* Critical section, we can not allowed signal here.  */
  sigemptyset (&signalset);
  sigaddset (&signalset, SIGTERM);
  sigaddset (&signalset, SIGHUP);
  sigaddset (&signalset, SIGTSTP);
  sigaddset (&signalset, SIGINT);
  sigaddset (&signalset, SIGWINCH);
  sigprocmask (SIG_BLOCK, &signalset, 0);

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
		if ((status = stream_read (mailbox->stream, buffer,
					   nread, offset, &n) != 0)
		    || fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
		  {
		    if (status == 0)
		      status = errno;
		    fprintf (stderr, "Error expunge:%d: (%s)\n", __LINE__,
			     strerror (status));
		    goto bailout0;
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
	      if ((status = stream_read (mailbox->stream, buffer, nread,
					 offset, &n) != 0)
		  || fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
		{
		  if (status == 0)
		    status = errno;
		  fprintf (stderr, "Error expunge:%d: %s", __LINE__,
			   strerror (status));
		  goto bailout0;
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
	    if ((status = stream_read (mailbox->stream, buffer, nread,
				       offset, &n) != 0)
		|| fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
	      {
		if (status == 0)
		  status = errno;
		fprintf (stderr, "Error expunge:%d: %s", __LINE__,
			 strerror (status));
		goto bailout0;
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
    if (stream_size (mailbox->stream, &size) == 0)
      {
	size_t n = 0;
	off_t offset = size;
	size_t len = size - mud->size;
	while (len > 0)
	  {
	    nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	    if ((status = stream_read (mailbox->stream, buffer, nread,
				       offset, &n) != 0)
		|| fwrite(buffer, sizeof(*buffer), n, tempfile) != n)
	      {
		if (status == 0)
		  status = errno;
		fprintf (stderr, "Error expunge:%d: %s", __LINE__,
			 strerror (status));
		goto bailout0;
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
	      status = stream_write (mailbox->stream, buffer, nread, offset, &n);
	      if (status != 0)
		{
		  fprintf (stderr, "Error expunge:%d: %s\n", __LINE__,
			   strerror (status));
		  goto bailout;
		}
	      nread -= n;
	      offset += n;
	    }
	}
    }

  /* How can I handle error here ??  */
  clearerr (tempfile);

  /* Flush/truncation.  */
  stream_flush (mailbox->stream);
  status = stream_truncate (mailbox->stream, total);
  if (status != 0)
    {
      fprintf (stderr, "Error expunging:%d: %s", __LINE__, strerror (status));
      goto bailout;
    }

 bailout0:
  /* Don't remove the tmp mbox in case of errors, when writing back.  */
  remove (tmpmbox);

 bailout:

  free (tmpmbox);
  /* Release the locks.  */
  mbox_unlock (mailbox);
  fclose (tempfile);
  sigprocmask (SIG_UNBLOCK, &signalset, 0);

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
	      if ((j + 1) >= mud->messages_count)
		{
		  /* Move all the pointers up.  So the message pointer
		     part of mum will be at the right position.  */
		  memmove (mud->umessages + j, mud->umessages + j + 1,
			   (dlast - dirty) * sizeof (mum));
		  mum->header_from = mum->header_from_end = 0;
		  mum->header_status = mum->header_status_end = 0;
		  mum->body = mum->body_end = 0;
		  mum->header_lines = mum->body_lines = 0;
		  /* We are not free()ing the useless mum, but instead
		     we put it back in the pool, to be reuse.  */
		  mud->umessages[dlast] = mum;
		  dlast--;
		  /* Set mum to the new value after the memmove so it
		     gets cleared to.  */
		  mum = mud->umessages[j];
		}
	    }
	  mum->header_from = mum->header_from_end = 0;
	  mum->header_status = mum->header_status_end = 0;
	  mum->body = mum->body_end = 0;
	  mum->header_lines = mum->body_lines = 0;
	}
      /* This is should reset the messages_count, the last argument 0 means
	 not to send event notification.  */
      mailbox_unlock (mailbox);
      mbox_scan0 (mailbox, dirty, NULL, 0);
    }
  else
    mailbox_unlock (mailbox);
  return status;
}

static int
mbox_get_body_fd (stream_t is, int *pfd)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  return mbox_get_fd (mum, pfd);
}

static int
mbox_get_hdr_fd (stream_t is, int *pfd)
{
  header_t header = stream_get_owner (is);
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  return mbox_get_fd (mum, pfd);
}

static int
mbox_get_fd (mbox_message_t mum, int *pfd)
{
  int status;
  if (mum == NULL)
    return EINVAL;
  mailbox_rdlock (mum->mud->mailbox);
  status = stream_get_fd (mum->stream, pfd);
  mailbox_unlock (mum->mud->mailbox);
  return status;
}

static int
mbox_get_attr_flags (attribute_t attr, int *pflags)
{
  message_t msg = attribute_get_owner (attr);
  mbox_message_t mum = message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;

  mailbox_rdlock (mum->mud->mailbox);
  if (pflags)
    *pflags = mum->new_flags;
  mailbox_unlock (mum->mud->mailbox);
  return 0;
}

static int
mbox_set_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  mbox_message_t mum = message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;

  mailbox_rdlock (mum->mud->mailbox);
  mum->new_flags |= flags;
  mailbox_unlock (mum->mud->mailbox);
  return 0;
}

static int
mbox_unset_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  mbox_message_t mum = message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;

  mailbox_rdlock (mum->mud->mailbox);
  mum->new_flags &= ~flags;
  mailbox_unlock (mum->mud->mailbox);
  return 0;
}

static int
mbox_readstream (stream_t is, char *buffer, size_t buflen,
		 off_t off, size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  size_t nread = 0;

  if (mum == NULL)
    return EINVAL;

  if (buffer == NULL || buflen == 0)
    {
      if (pnread)
	*pnread = nread;
      return 0;
    }

  mailbox_rdlock (mum->mud->mailbox);
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
	  {
	    mailbox_unlock (mum->mud->mailbox);
	    return status;
	  }
      }
  }
  mailbox_unlock (mum->mud->mailbox);

  if (pnread)
    *pnread = nread;
  return 0;
}

static int
mbox_get_header_read (stream_t is, char *buffer, size_t len,
		      off_t off, size_t *pnread)
{
  header_t header = stream_get_owner (is);
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  size_t nread = 0;
  int status = 0;
  off_t ln;

  if (mum == NULL)
    return EINVAL;

  mailbox_rdlock (mum->mud->mailbox);
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
  mailbox_unlock (mum->mud->mailbox);
  return status;
}

static int
mbox_header_size (header_t header, size_t *psize)
{
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  mailbox_rdlock (mum->mud->mailbox);
  if (psize)
    *psize = mum->body - mum->header_from_end;
  mailbox_unlock (mum->mud->mailbox);
  return 0;
}

static int
mbox_header_lines (header_t header, size_t *plines)
{
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  mailbox_rdlock (mum->mud->mailbox);
  if (plines)
    *plines = mum->header_lines;
  mailbox_unlock (mum->mud->mailbox);
  return 0;
}

static int
mbox_body_size (body_t body, size_t *psize)
{
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  if (psize)
    *psize = mum->body_end - mum->body + 1;
  return 0;
}

static int
mbox_body_lines (body_t body, size_t *plines)
{
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  mailbox_rdlock (mum->mud->mailbox);
  if (plines)
    *plines = mum->body_lines;
  mailbox_unlock (mum->mud->mailbox);
  return 0;
}

static int
mbox_msg_received (message_t msg, char *buf, size_t len,
			   size_t *pnwrite)
{
  mbox_message_t mum = message_get_owner (msg);
  size_t n = 0;
  int status;
  char buffer[512];

  if (mum == NULL)
    return EINVAL;

  mailbox_rdlock (mum->mud->mailbox);
  status = stream_readline (mum->stream, buffer, sizeof(buffer),
			    mum->header_from, &n);
  mailbox_unlock (mum->mud->mailbox);
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
mbox_msg_from (message_t msg, char *buf, size_t len, size_t *pnwrite)
{
  mbox_message_t mum = message_get_owner (msg);
  size_t n = 0;
  int status;
  char buffer[512];

  if (mum == NULL)
    return EINVAL;

  mailbox_rdlock (mum->mud->mailbox);
  status = stream_readline (mum->stream, buffer, sizeof(buffer),
			    mum->header_from, &n);
  mailbox_unlock (mum->mud->mailbox);
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
mbox_get_message (mailbox_t mailbox, size_t msgno, message_t *pmsg)
{
  int status;
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  message_t msg = NULL;

  /* Sanity checks.  */
  if (pmsg == NULL || mud == NULL || (!(mud->messages_count > 0 && msgno > 0
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

  MAILBOX_DEBUG2 (mailbox, MU_DEBUG_TRACE, "mbox_get_message(%s, %d)\n",
		  mud->name, msgno);

  /* Get an empty message struct.  */
  status = message_create (&msg, mum);
  if (status != 0)
    return status;

  /* Set the header.  */
  {
    header_t header = NULL;
    stream_t stream = NULL;
    if ((status = header_create (&header, NULL, 0, msg)) != 0
	|| (status = stream_create (&stream, mailbox->flags, header)) != 0)
      {
	stream_destroy (&stream, header);
	header_destroy (&header, msg);
	message_destroy (&msg, mum);
	return status;
      }
    stream_set_read (stream, mbox_get_header_read, header);
    stream_set_fd (stream, mbox_get_hdr_fd, header);
    header_set_stream (header, stream, msg);
    header_set_size (header, mbox_header_size, msg);
    header_set_lines (header, mbox_header_lines, msg);
    message_set_header (msg, header, mum);
  }

  /* Set the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, msg);
    if (status != 0)
      {
	message_destroy (&msg, mum);
	return status;
      }
    mum->new_flags = mum->old_flags;
    attribute_set_get_flags (attribute, mbox_get_attr_flags, msg);
    attribute_set_set_flags (attribute, mbox_set_attr_flags, msg);
    attribute_set_unset_flags (attribute, mbox_unset_attr_flags, msg);
    message_set_attribute (msg, attribute, mum);
  }

  /* Prepare the body.  */
  {
    body_t body = NULL;
    stream_t stream = NULL;
    if ((status = body_create (&body, msg)) != 0
	|| (status = stream_create (&stream, mailbox->flags, body)) != 0)
      {
	body_destroy (&body, msg);
	stream_destroy (&stream, body);
	message_destroy (&msg, mum);
	return status;
      }
    stream_set_read (stream, mbox_readstream, body);
    stream_set_fd (stream, mbox_get_body_fd, body);
    body_set_stream (body, stream, msg);
    body_set_size (body, mbox_body_size, msg);
    body_set_lines (body, mbox_body_lines, msg);
    message_set_body (msg, body, mum);
  }

  /* Set the envelope.  */
  message_set_from (msg, mbox_msg_from, mum);
  message_set_received (msg, mbox_msg_received, mum);

  /* Attach the message to the mailbox mbox data.  */
  mum->message = msg;

  if (pmsg)
    *pmsg = msg;
  return 0;
}

static int
mbox_append_message (mailbox_t mailbox, message_t msg)
{
  mbox_data_t mud = mailbox->data;
  if (msg == NULL || mud == NULL)
    return EINVAL;

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_append_message (%s)\n",
		  mud->name);

  mailbox_wrlock (mailbox);
  mbox_lock (mailbox, MU_LOCKER_WRLOCK);
  {
    off_t size;
    int status;
    size_t n = 0;
    char nl = '\n';

    /* Move to the end of the file, not necesary if _APPEND mode.  */
    status = stream_size (mailbox->stream, &size);
    if (status != 0)
      {
	mailbox_unlock (mailbox);
	mbox_unlock (mailbox);
	return status;
      }

    switch (mud->state)
      {
      case MBOX_NO_STATE:
	mud->from = calloc (128, sizeof (char));
	if (mud->from == NULL)
	  {
	    mbox_unlock (mailbox);
	    mailbox_unlock (mailbox);
	    return ENOMEM;
	  }
	mud->date = calloc (128, sizeof (char));
	if (mud->date == NULL)
	  {
	    free (mud->from);
	    mud->from = NULL;
	    mud->state = MBOX_NO_STATE;
	    mbox_unlock (mailbox);
	    mailbox_unlock (mailbox);
	    return ENOMEM;
	  }
	mud->off = 0;
	mud->state = MBOX_STATE_FROM;

      case MBOX_STATE_FROM:
	/* Generate a "From " separator.  */
	{
	  char *s;
	  size_t len = 0;
	  status = message_from (msg, mud->from, 127, &len);
	  if (status != 0)
	    {
	      if (status != EAGAIN)
		{
		  free (mud->from);
		  free (mud->date);
		  mud->date = mud->from = NULL;
		  mud->state = MBOX_NO_STATE;
		  mbox_unlock (mailbox);
		}
	      mailbox_unlock (mailbox);
	      return status;
	    }
	  /* Nuke trailing newline.  */
	  s = memchr (mud->from, nl, len);
	  if (s)
	    *s = '\0';
	  mud->state = MBOX_STATE_DATE;
	}

      case MBOX_STATE_DATE:
	/* Generate a date for the "From "  separator.  */
	{
	  char *s;
	  size_t len = 0;
	  status = message_received (msg, mud->date, 127, &len);
	  if (status != 0)
	    {
	      if (status != EAGAIN)
		{
		  free (mud->from);
		  free (mud->date);
		  mud->date = mud->from = NULL;
		  mud->state = MBOX_NO_STATE;
		  mbox_unlock (mailbox);
		}
	      mailbox_unlock (mailbox);
	      return status;
	    }
	  /* Nuke trailing newline.  */
	  s = memchr (mud->date, nl, len);
	  if (s)
	    *s = '\0';
	  /* Write the separator to the mailbox.  */
	  stream_write (mailbox->stream, "From ", 5, size, &n);
	  size += n;
	  stream_write (mailbox->stream, mud->from, strlen (mud->from), size, &n);
	  size += n;
	  stream_write (mailbox->stream, " ", 1, size, &n);
	  size += n;
	  stream_write (mailbox->stream, mud->date, strlen (mud->date), size, &n);
	  size += n;
	  stream_write (mailbox->stream, &nl , 1, size, &n);
	  size += n;
	  free (mud->from);
	  free (mud->date);
	  mud->from = mud->date = NULL;
	  mud->state = MBOX_STATE_APPEND;
	}

      case MBOX_STATE_APPEND:
	/* Append the Message.  */
	{
	  char buffer[BUFSIZ];
	  size_t nread = 0;
	  stream_t is;
	  message_get_stream (msg, &is);
	  do
	    {
	      status = stream_read (is, buffer, sizeof (buffer), mud->off,
				    &nread);
	      if (status != 0)
		{
		  if (status != EAGAIN)
		    {
		      free (mud->from);
		      free (mud->date);
		      mud->date = mud->from = NULL;
		      mud->state = MBOX_NO_STATE;
		      mbox_unlock (mailbox);
		    }
		  stream_flush (mailbox->stream);
		  mailbox_unlock (mailbox);
		  return status;
		}
	      stream_write (mailbox->stream, buffer, nread, size, &n);
	      mud->off += nread;
	      size += n;
	    }
	  while (nread > 0);
	  stream_write (mailbox->stream, &nl, 1, size, &n);
	}

      default:
	break;
      }
  }
  stream_flush (mailbox->stream);
  mud->state = MBOX_NO_STATE;
  mbox_unlock (mailbox);
  mailbox_unlock (mailbox);
  return 0;
}

static int
mbox_size (mailbox_t mailbox, off_t *psize)
{
  off_t size;
  int status;

  /* Maybe was not open yet ??  */
  mailbox_rdlock (mailbox);
  status  = stream_size (mailbox->stream, &size);
  mailbox_unlock (mailbox);
  if (status != 0)
    return status;
  if (psize)
    *psize = size;
  return 0;
}

static int
mbox_messages_count (mailbox_t mailbox, size_t *pcount)
{
  mbox_data_t mud = mailbox->data;
  if (mud == NULL)
    return EINVAL;

  if (! mbox_is_updated (mailbox))
    return mbox_scan0 (mailbox,  1, pcount, 1);

  if (pcount)
    *pcount = mud->messages_count;

  return 0;
}

/* Locking.  */
static int
mbox_lock (mailbox_t mailbox, int flag)
{
  if (mailbox && mailbox->locker != NULL)
    {
      locker_t locker = mailbox->locker;
      locker_lock (locker, flag);
    }
  return 0;
}

static int
mbox_touchlock (mailbox_t mailbox)
{
  if (mailbox && mailbox->locker != NULL)
    {
      locker_t locker = mailbox->locker;
      locker_touchlock (locker);
    }
  return 0;
}

static int
mbox_unlock (mailbox_t mailbox)
{
  if (mailbox && mailbox->locker != NULL)
    {
      locker_t locker = mailbox->locker;
      locker_unlock (locker);
    }
  return 0;
}
