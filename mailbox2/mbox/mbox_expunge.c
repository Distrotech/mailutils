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
static int
mbox_expunge0 (mbox_t mbox, int remove_deleted)
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
  off_t tmp_size = 0;
  size_t save_uidvalidity = 0; /* uidvalidity is save in the first message.  */

  mbox_debug_print (mbox, "expunge, r=%d", remove_deleted);
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
      mbox_error_cb (mbox, errno);
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

    /* Must be flag CREATE if not the mailbox_open will/may try to mmap()
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
      mbox_error_cb (mbox, status);
      return status;
    }

  /* Critical section, can not allowed signal here.  */
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
	  /* We save the uidvalidity in the first message, if it is being
             deleted we need to move the uidvalidity to the first available
             (non-deleted) message.  */
          if (i == save_uidvalidity)
            {
              save_uidvalidity = i + 1;
            }
	  continue;
	}
      else
	{
	  stream_t hstream = NULL, bstream = NULL;
	  attribute_t attribute = NULL;
	  char *sep = NULL;
	  /* The message was not instanciated, create one here.  */
	  if (mum->separator == NULL || mum->attribute
	      || mum->header.stream == NULL || mum->body.stream == NULL)
	    {
	      if ((status = mbox_get_separator (mbox, i + 1, &sep) != 0)
		  || (status = mbox_get_attribute (mbox, i + 1, &attribute) != 0)
		  || (status = mbox_get_hstream (mbox, i + 1, &hstream) != 0)
		  || (status = mbox_get_bstream (mbox, i + 1, &bstream) != 0))
		{
		  mbox_error_cb (mbox, status);
		  goto bailout0;
		}
	    }
	  status = mbox_append_hb0 (tmp_mbox, mum->separator, mum->attribute,
				    mum->header.stream, mum->body.stream,
				    save_uidvalidity, mum->uid);
	  free (sep);
	  attribute_destroy (&attribute);
	  stream_destroy (&hstream);
	  stream_destroy (&bstream);
	  if (status != 0)
	    {
	      mbox_error_cb (mbox, status);
	      goto bailout0;
	    }
	  /* Clear the dirty bits.  */
	  mum->attr_flags &= ~MU_ATTRIBUTE_MODIFIED;
	}
    } /* for (;;) */

  /* Get the real size of the mailbox.  The size maintain in
     the struct _mailbox { off_t size; }(the one return in
     mailbox_get_size()) only returns the size that mbox_scan()
     is aware of, not necessary the size of the file after an append.  */
  stream_get_size (tmp_mbox->carrier, &tmp_size);

  /* Caution: before ftruncate()ing the file see
     - if we've receive new mails.  Some programs may not respect the lock,
     - or the lock was held for too long.
     - The mailbox may not have been properly updated before expunging.  */
  {
    off_t size = 0;
    if (stream_get_size (mbox->carrier, &size) == 0)
      {
	off_t len = size - mbox->size;
	off_t offset = mbox->size;
	char buf [1024];
	size_t n = 0;
	if (len > 0)
	  {
	    while ((status = stream_read (mbox->carrier, buf, sizeof buf,
					  offset, &n)) == 0 && n > 0)
	      {
		status = stream_write(tmp_mbox->carrier, buf, n, tmp_size, &n);
		if (status != 0)
		  {
		    mbox_error_cb (mbox, status);
		    goto bailout0;
		  }
		offset += n;
		tmp_size += n;
	      }
	  }
	else if (len < 0)
	  {
	    /* Corrupted mailbox.  */
	    mbox_error_cb (mbox, status);
	    goto bailout0;
	  }
      }
  } /* End of precaution.  */

  /* Seek and rewrite it.  */
  if (tmp_size > 0)
    {
      char buf [1024];
      size_t n = 0;
      off_t tmp_off = 0;
      off_t offset = marker;

      while ((status = stream_read (tmp_mbox->carrier, buf, sizeof buf,
				    tmp_off, &n)) == 0 && n > 0)
	{
	  status = stream_write (mbox->carrier, buf, n, offset,  &n);
	  if (status != 0)
	    {
	      mbox_error_cb (mbox, status);
	      goto bailout;
	    }
	  tmp_off += n;
	  offset += n;
	}
    }

  /* Flush/truncation. Need to flush before truncate.  */
  stream_flush (mbox->carrier);
  status = stream_truncate (mbox->carrier, tmp_size + marker);
  if (status != 0)
    {
      mbox_error_cb (mbox, status);
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
	  mum = mbox->umessages[j];
	  /* Clear all the references to streams.  */
	  if (remove_deleted && ATTRIBUTE_IS_DELETED (mum->attr_flags))
	    {
	      mbox_release_msg (mbox, j + 1);
	      if ((j + 1) <= dlast)
		{
		  /* Move all the pointers up.  So the message pointers
		     will be at the right position.  */
		  memmove (mbox->umessages + j, mbox->umessages + j + 1,
			   (dlast - j) * sizeof mum);
		  dlast--;
		}
	      mbox->umessages_count--;
	      mbox->messages_count--;
	    }
	  else
	    {
	      /* Readjust the offsets and attach objects.  */
	      mum->from_ = mum->header.start = 0;
	      mum->body.start = mum->body.end = 0;
	      mum->header.lines = mum->body.lines = 0;
	      stream_mbox_set_msgno (mum->header.stream, j + 1);
	      stream_mbox_set_msgno (mum->body.stream, j + 1);
	      attribute_mbox_set_msgno (mum->attribute, j + 1);
	    }
	}

      /* Must Realloc the right size before the scan. some slots
	 in the umessages may be NULL after remove_deleted.  */
      mbox->umessages  = realloc (mbox->umessages,
				  mbox->umessages_count
				  * sizeof (*(mbox->umessages)));

      /* This should reset the messages_count, the last argument 0 means
	 not to send event notification.  */
      mbox_scan0 (mbox, dirty, NULL, 0);
    }
  return status;
}

int
mbox_expunge (mbox_t mbox)
{
  return mbox_expunge0 (mbox, 1);
}

int
mbox_save (mbox_t mbox)
{
  return mbox_expunge0 (mbox, 0);
}
