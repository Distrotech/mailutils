/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifdef WITH_PTHREAD
# ifdef HAVE_PTHREAD_H
#  define _XOPEN_SOURCE  500
#  include <pthread.h>
# endif
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

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

#define ATTRIBUTE_IS_DELETED(flag)        (flag & MU_ATTRIBUTE_DELETED)
#define ATTRIBUTE_IS_MODIFIED(flag)        (flag & MU_ATTRIBUTE_MODIFIED)
#define ATTRIBUTE_IS_EQUAL(flag1, flag2)  (flag1 == flag2)

static int
mbox_tmpfile (mbox_t mbox, char **pbox)
{
  const char *tmpdir;
  int fd;
  const char *basename;

  /*  P_tmpdir should be in <stdio.h>.  */
#ifndef P_tmpdir
#  define P_tmpdir "/tmp"
#endif

  basename = strrchr (mbox->filename, '/');
  if (basename)
    basename++;
  else
    basename = mbox->filename;

  tmpdir =  getenv ("TMPDIR") ? getenv ("TMPDIR") : P_tmpdir;
  /* (separator + null) == 2 + XXXXXX == 6 + ... */
  *pbox = calloc (strlen (tmpdir) + /* '/' */ 1 + /*strlen ("MU_")*/ 3 +
                  strlen (basename) + /*strlen ("_XXXXXX")*/ 7 + /*null*/1,
                  sizeof (**pbox));
  if (*pbox == NULL)
    return -1;
  sprintf (*pbox, "%s/MU_%s_XXXXXX", tmpdir, basename);
#ifdef HAVE_MKSTEMP
  fd = mkstemp (*pbox);
#else
  /* Create the file.  It must not exist.  If it does exist, fail.  */
  if (mktemp (*pbox))
    {
      fd = open (*pbox, O_RDWR|O_CREAT|O_EXCL, 0600);
    }
  else
    {
      free (*pbox);
      fd = -1;
    }
#endif
  return fd;
}

/* For the expunge bits  we took a very cautionnary approach, meaning
   we create a temporary mailbox in the tmpdir copy all the message not mark
   deleted(Actually we copy all the message that may have been modified
   i.e new header values set; UIDL or UID or etc ....
   and skip the deleted ones, truncate the real mailbox to the desired size
   and overwrite with the tmp mailbox.  The approach to do everyting
   in core is tempting but require
   - to much memory, it is not rare nowadays to have 30 Megs mailbox,
   - also there is danger for filesystems with quotas,
   - if we run out of disk space everything is lost.
   - or some program may not respect the advisory lock and decide to append
   a new message while your expunging etc ...
   The real downside to the approach is that when things go wrong
   the temporary file may be left in /tmp, which is not all that bad
   because at least, we have something to recuperate when failure.  */
int
mbox_expunge (mbox_t mbox, int remove_deleted)
{
  mbox_message_t mum;
  int status = 0;
  sigset_t signalset;
  size_t i, j, dirty;  /* dirty will indicate the first modified message.  */
  off_t marker = 0;    /* marker will be the position to truncate.  */
  off_t total = 0;
  int tmp_fd;
  char *tmp_name = NULL;
  mbox_t tmp_mbox = NULL;

  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* Noop.  */
  if (mbox->messages_count == 0)
    return 0;

  /* Find the first dirty(modified) message.  */
  for (dirty = 0; dirty < mbox->messages_count; dirty++)
    {
      mum = mbox->umessages[dirty];
      /* Message may have been tampered, break here.  */
      if (ATTRIBUTE_IS_MODIFIED (mum->attr_flags))
	break;
    }

  /* Did something change ?  */
  if (dirty == mbox->messages_count)
    return 0; /* Nothing change, bail out.  */

  /* Create a temporary file.  */
  tmp_fd = mbox_tmpfile (mbox, &tmp_name);
  if (tmp_fd == -1)
    {
      if (tmp_name)
	free (tmp_name);
      /* mu_error ("Failed to create temporary file when expunging.\n"); */
      return errno;
    }

  /* Create temporary mailbox_t.  */
  {
    status = mbox_create (&tmp_mbox);
    if (status != 0)
      {
	close (tmp_fd);
	remove (tmp_name);
	free (tmp_name);
	return status;
      }

    /* Must be flag CREATE if not the mailbox_open will try to mmap()
       the file.  */
    status = mbox_open (tmp_mbox, tmp_name, MU_STREAM_CREAT | MU_STREAM_RDWR);
    if (status != 0)
      {
	close (tmp_fd);
	remove (tmp_name);
	free (tmp_name);
	return status;
      }
    close (tmp_fd); /* This one is useless the mailbox have its own.  */
    /* May need when appending.  */
    tmp_mbox->uidvalidity = mbox->uidvalidity;
    tmp_mbox->uidnext = mbox->uidnext;
  }

  /* Get the File lock.  */
  status = lockfile_lock (mbox->lockfile);
  if (status != 0)
    {
      mbox_close (tmp_mbox);
      mbox_destroy (&tmp_mbox);
      remove (tmp_name);
      free (tmp_name);
      /* mu_error ("Failed to grab the lock\n"); */
      return status;
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
  marker = mbox->umessages[dirty]->from_;
  total = 0;

  /* Copy to the temporary mailbox emails not mark deleted.  */
  for (i = dirty; i < mbox->messages_count; i++)
    {
      mum = mbox->umessages[i];

      /* Skip it, if mark for deletion.  */
      if (remove_deleted && ATTRIBUTE_IS_DELETED (mum->attr_flags))
	{
	  continue;
	}

      {
	stream_t hstream = NULL, bstream = NULL;
	char *sep = NULL;
	/* The message was not instanciated, probably the dirty flag was
	   set by mbox_scan(), create one here.  */
	if (mum->separator == NULL || mum->header.stream == NULL
	    || mum->body.stream == NULL)
	  {
	    if (mbox_get_hstream (mbox, i + 1, &hstream) != 0
		|| mbox_get_bstream (mbox, i + 1, &bstream) != 0
		|| mbox_get_separator (mbox, i + 1, &sep) != 0)
	      {
		/* mu_error ("Error expunge:%d", __LINE__); */
		goto bailout0;
	      }
	  }
	status = mbox_append_hb (tmp_mbox, mum->separator, mum->header.stream,
				 mum->body.stream);
	if (sep)
	  free (sep);
	stream_destroy (&hstream);
	stream_destroy (&bstream);
	if (status != 0)
	  {
	    /* mu_error ("Error expunge:%d: %s", __LINE__,
	       strerror (status)); */
	    goto bailout0;
	  }
	/* Clear the dirty bits.  */
	mum->attr_flags &= ~MU_ATTRIBUTE_MODIFIED;
      }
    } /* for (;;) */

  /* Caution: before ftruncate()ing the file see
     - if we've receive new mails.  Some programs may not respect the lock,
     - or the lock was held for too long.
     - The mailbox may not have been properly updated before expunging.  */
  {
    off_t size = 0;
    if (stream_get_size (mbox->carrier, &size) == 0)
      {
	off_t len = size - mbox->size;
	char buffer [1024];
	size_t n = 0;
	if (len > 0)
	  {
	    stream_seek (mbox->carrier, mbox->size, MU_STREAM_WHENCE_SET);
	    stream_seek (mbox->carrier, tmp_mbox->size, MU_STREAM_WHENCE_SET);
	    while ((status = stream_read (mbox->carrier, buffer,
					  sizeof buffer, &n)) == 0 && n > 0)
	      {
		status = stream_write (tmp_mbox->carrier, buffer, n, &n);
		if (status != 0)
		  {
		    /* mu_error ("Error expunge:%d: %s", __LINE__,
		       strerror (status)); */
		    goto bailout0;
		  }
	      }
	  }
	else if (len < 0)
	  {
	    /* Corrupted mailbox.  */
	    /* mu_error ("Error expunge:%d: %s", __LINE__,
	       strerror (status)); */
	    goto bailout0;
	  }
      }
  } /* End of precaution.  */

  /* Seek and rewrite it.  */
  if (total > 0)
    {
      char buffer [1024];
      size_t n = 0;

      stream_seek (mbox->carrier, marker, MU_STREAM_WHENCE_SET);
      stream_seek (tmp_mbox->carrier, 0, MU_STREAM_WHENCE_SET);
      while ((status = stream_read (tmp_mbox->carrier, buffer,
				    sizeof buffer, &n)) == 0 && n > 0)
	{
	  status = stream_write (mbox->carrier, buffer, n, &n);
	  if (status != 0)
	    {
	      /* mu_error ("Error expunge:%d: %s\n", __LINE__,
		 strerror (status)); */
	      goto bailout;
	    }
	}
    }

  /* Flush/truncation. Need to flush before truncate.  */
  stream_flush (mbox->carrier);
  status = stream_truncate (mbox->carrier, tmp_mbox->size + marker);
  if (status != 0)
    {
      /* mu_error ("Error expunging:%d: %s\n", __LINE__,
	 strerror (status)); */
      goto bailout;
    }

  /* Don't remove the tmp mbox in case of errors, when writing back.  */
 bailout0:
  remove (tmp_name);

 bailout:

  free (tmp_name);
  /* Release the File lock.  */
  lockfile_unlock (mbox->lockfile);
  mbox_close (tmp_mbox);
  mbox_destroy (&tmp_mbox);

  /* Reenable signal.  */
  sigprocmask (SIG_UNBLOCK, &signalset, 0);

  /* We need to readjust the pointers.
     It is a little hairy because we need to keep the message pointers alive
     So we are going through the array and "defragmentize".  For example
     in (1 2 3 4) if 2 was deleted we need to move 3 4 up by one etc ..
  */
  if (status == 0)
    {
      size_t dlast;
      for (j = dirty, dlast = mbox->messages_count - 1; j <= dlast; j++)
	{
	  /* Clear all the references, any attach messages been already
	     destroy above.  */
	  mum = mbox->umessages[j];
	  if (remove_deleted && ATTRIBUTE_IS_DELETED (mum->attr_flags))
	    {
	      if ((j + 1) <= dlast)
		{
		  /* Move all the pointers up.  So the message pointer
		     part of mum will be at the right position.  */
		  memmove (mbox->umessages + j, mbox->umessages + j + 1,
			   (dlast - j) * sizeof (mum));
#if 1
		  mum->from_ = mum->header.start = 0;
		  mum->body.start = mum->body.end = 0;
		  mum->header.lines = mum->body.lines = 0;
#endif
		  mbox_hcache_free (mbox, i + 1);
		  memset (mum, 0, sizeof (*mum));
		  /* We are not free()ing the useless mum, but instead
		     we put it back in the pool, to be reuse.  */
		  mbox->umessages[dlast] = mum;
		  dlast--;
		  /* Set mum to the new value after the memmove so it
		     gets cleared to.  */
		  mum = mbox->umessages[j];
		}
	      else
		{
		  mbox_hcache_free (mbox, i + 1);
		  memset (mum, 0, sizeof (*mum));
		}
	    }
	  mum->from_ = mum->header.start = 0;
	  mum->body.start = mum->body.end = 0;
	  mum->header.lines = mum->body.lines = 0;
	  mbox_hcache_free (mbox, i + 1);
	}
      /* This is should reset the messages_count, the last argument 0 means
	 not to send event notification.  */
      mbox_scan (mbox, dirty, NULL, 0);
    }
  return status;
}
