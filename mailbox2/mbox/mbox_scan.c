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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mailutils/error.h>
#include <mailutils/attribute.h>
#include <mailutils/sys/mbox.h>
#include "valid.h"

#define ISSTATUS(buf) (\
(buf[0] == 'S' || buf[0] == 's') \
 && (buf[1] == 'T' || buf[1] == 't') \
 && (buf[2] == 'A' || buf[2] == 'a') \
 && (buf[3] == 'T' || buf[3] == 't') \
 && (buf[4] == 'U' || buf[4] == 'u') \
 && (buf[5] == 'S' || buf[5] == 's') \
 && (buf[6] == ':' || buf[6] == ' ' || buf[6] == '\t'))

#define ISX_IMAPBASE(buf) (\
(buf[0] == 'X' || buf[0] == 'x') \
 && (buf[1] == '-') \
 && (buf[2] == 'I' || buf[2] == 'i') \
 && (buf[3] == 'M' || buf[3] == 'm') \
 && (buf[4] == 'A' || buf[4] == 'a') \
 && (buf[5] == 'P' || buf[5] == 'p') \
 && (buf[6] == 'B' || buf[6] == 'b') \
 && (buf[7] == 'A' || buf[7] == 'a') \
 && (buf[8] == 'S' || buf[8] == 's') \
 && (buf[9] == 'E' || buf[9] == 'e') \
 && (buf[10] == ':' || buf[10] == ' ' || buf[10] == '\t'))

#define ISX_UID(buf) (\
(buf[0] == 'X' || buf[0] == 'x') \
 && (buf[1] == '-') \
 && (buf[2] == 'U' || buf[2] == 'u') \
 && (buf[3] == 'I' || buf[3] == 'i') \
 && (buf[4] == 'D' || buf[4] == 'd') \
 && (buf[5] == ':' || buf[5] == ' ' || buf[5] == '\t'))


static int
mbox_parse_status (mbox_message_t mum, char *buf, size_t n)
{
  char *s = memchr (buf, ':', n);
  if (s)
    {
      s++;
      for (; *s; s++)
	{
	  switch (*s)
	    {
	    case 'r':
	    case 'R':
	      mum->attr_flags |= MU_ATTRIBUTE_READ;
	      break;
	    case 'O':
	    case 'o':
	      mum->attr_flags |= MU_ATTRIBUTE_SEEN;
	      break;
	    case 'a':
	    case 'A':
	      mum->attr_flags |= MU_ATTRIBUTE_ANSWERED;
	      break;
	    case 'd':
	    case 'D':
	      mum->attr_flags |= MU_ATTRIBUTE_DELETED;
	      break;
	    }
	}
    }
  return 0;
}

static int
mbox_alloc_umessages (mbox_t mbox)
{
  mbox_message_t mum;
  unsigned int msgno = 0;
  if (mbox->messages_count >= mbox->umessages_count)
    {
      mbox_message_t *m;
      size_t i;
      size_t num = mbox->messages_count + 32;
      m = realloc (mbox->umessages, num * sizeof (*m));
      if (m == NULL)
	return MU_ERROR_NO_MEMORY;
      mbox->umessages = m;
      mbox->umessages_count = num;
      for (i = mbox->messages_count; i < mbox->umessages_count; i++)
	{
	  mbox->umessages[i] = calloc (1, sizeof (**m));
	  if (mbox->umessages[i] == NULL)
	    return MU_ERROR_NO_MEMORY;
	}
    }
  if (mum->separator)
    free (mum->separator);
  mum->separator = 0;

  mbox_hcache_free (mbox, msgno + 1);
  return 0;
}

/* Parsing.
   The approach is to detect the "From " at the start of a new message, give
   the position of the header and scan until "\n" then set header and body
   position, scan until we it another "From ".  */

int
mbox_scan (mbox_t mbox, unsigned int msgno, unsigned int *pcount, int do_notif)
{
#define MSGLINELEN 1024
  char buf[MSGLINELEN];
  int inheader;
  int inbody;
  off_t total = 0;
  mbox_message_t mum = NULL;
  int status = 0;
  unsigned int lines;
  int newline;
  size_t n = 0;
  char *sfield = NULL;
  int fd = -1;
  off_t file_size = 0;

  int zn, isfrom = 0;
  char *temp;

  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* Save the timestamp.  */
  status = stream_get_fd (mbox->carrier, &fd);
  if (status == 0)
    {
      struct stat statbuf;
      if (fstat (fd, &statbuf) == 0)
	{
	  /* We do not save the size here since the scan may be interrupted
	     we'll have a partial scan mailbox which does not reflect the
	     reality if expunge.  */
	  /*	  mbox->size = statbuf.st_size; */
	  file_size = statbuf.st_size;
	}
      else
	status = MU_ERROR_IO;
    }

  /* Move along, move along nothing to see.  */
  if (!mbox_has_newmail (mbox) && mbox->size == file_size)
    return 0;

  /* Bailout early on error.  */
  if (status != 0)
    return status;

  if (mbox->hcache.size == 0)
    mbox_set_hcache_default (mbox);

  /* Lock the mbox before starting.  */
  /* FIXME: Check error..  */
  lockfile_lock (mbox->lockfile);

  /* Seek to the starting point.  */
  if (mbox->umessages && msgno > 0 && mbox->messages_count > 0
      && msgno <= mbox->messages_count)
    {
      mum = mbox->umessages[msgno - 1];
      if (mum)
        total = mum->from_;
      mbox->messages_count = msgno - 1;
    }
  else
    mbox->messages_count = 0;

  newline = 1;
  errno = lines = inheader = inbody = 0;
  mum = NULL;

  status = stream_seek (mbox->carrier, total, MU_STREAM_WHENCE_SET);
  if (status != 0)
    {
      lockfile_unlock (mbox->lockfile);
      return status;
    }

  while ((status = stream_readline (mbox->carrier, buf, sizeof buf, &n)) == 0
	 && n > 0)
    {
      int nl;
      total += n;

      nl = (*buf == '\n') ? 1 : 0;
      VALID(buf, temp, isfrom, zn);
      isfrom = (isfrom) ? 1 : 0;

      /* Which part of the message are we in ?  */
      inheader = isfrom | ((!nl) & inheader);
      inbody = (!isfrom) & (!inheader);

      if (buf[n - 1] == '\n')
        lines++;

      if (inheader)
        {
          /* New message.  */
          if (isfrom)
            {
              /* Signal the end of the previous body.  */
              if (mum && !mum->body.end)
                {
                  mum->body.end = total - n;
                  mum->body.lines = --lines - newline;
		  /* DISPATCH_ADD_MSG(mbox); */
                  if (do_notif)
		    {
		      status = mbox_newmsg_cb (mbox, mbox->messages_count);
		      if (status)
			{
			  lockfile_unlock (mbox->lockfile);
			  return status;
			}
		      stream_seek (mbox->carrier, total, MU_STREAM_WHENCE_SET);
		    }
                }
              /* Allocate_msgs will initialize mum.  */
              status = mbox_alloc_umessages (mbox);
	      if (status != 0)
		{
		  lockfile_unlock (mbox->lockfile);
		  return status;
		}
              mbox->messages_count++;
              mum = mbox->umessages[mbox->messages_count - 1];
              mum->from_ = total - n;
              mum->header.start = total;
              mum->body.end = mum->body.start = 0;
              mum->attr_flags = 0;
              lines = 0;
            }
          else if (ISSTATUS(buf))
            {
	      mbox_parse_status (mum, buf, n);
	    }
	  else if (ISX_IMAPBASE(buf))
	    {
              char *s = memchr (buf, ':', n);
              if (s)
                {
                  s++;
		  mbox->uidvalidity = strtoul (s, &s, 10);
		  mbox->uidnext = strtoul (s, NULL, 10);
                }
            }
          else if (ISX_UID(buf))
            {
              char *s = memchr (buf, ':', n);
              if (s)
                {
                  s++;
                  mum->uid = strtoul(s, NULL, 10);
                }
            }
          else if (sfield && (buf[0] == ' ' || buf[0] == '\t'))
            {
	      if (buf[n - 1] == '\n')
		buf[n - 1] = 0;
              mbox_hcache_append (mbox, mbox->messages_count, sfield, buf);
            }
          else
            {
              char *s = memchr (buf, ':', n);
	      if (sfield)
		free (sfield);
	      sfield = NULL;
              if (s)
                {
		  *s = '\0';
                  s++;
		  if (buf[n - 1] == '\n')
		    buf[n - 1] = 0;
		  mbox_hcache_append (mbox, mbox->messages_count, buf, s);
		  sfield = strdup (buf);
		}
            }
        }

      /* Body.  */
      if (inbody)
        {
          /* Set the body position.  */
          if (mum && !mum->body.start)
            {
              mum->body.start = total - n + nl;
              mum->header.end = total - n + nl;
              mum->header.lines = lines;
              lines = 0;
            }
        }

      newline = nl;

      /* Every 100 mesgs update the lock, it should be every minute.  */
      if ((mbox->messages_count % 100) == 0)
        lockfile_touchlock (mbox->lockfile);

      /* Ping them every 1000 lines. Should be tunable.  */
      if (do_notif)
        if (((lines +1) % 1000) == 0)
          mbox_progress_cb (mbox, (total/file_size)*100);

    } /* while */

  if (sfield)
    free (sfield);

  /* FIXME: use mbox->size for total. */
  mbox->size = total;

  if (mum)
    {
      mum->body.end = total - newline;
      mum->body.lines = lines - newline;
      if (do_notif)
        mbox_newmsg_cb (mbox, mbox->messages_count);
    }
  if (pcount)
    *pcount = mbox->messages_count;
  lockfile_unlock (mbox->lockfile);

  /* Reset the uidvalidity.  */
  if (mbox->messages_count > 0)
    {
      mum = mbox->umessages[0];
      if (mbox->uidvalidity == 0)
        {
          mbox->uidvalidity = (unsigned long)time (NULL);
          mbox->uidnext = mbox->messages_count + 1;
          /* Tell that we have been modified for expunging.  */
          mum->attr_flags |= MU_ATTRIBUTE_MODIFIED;
        }
    }
  /* Reset the IMAP uids, if necessary. UID according to IMAP RFC is a 32 bit
     ascending number for each messages  */
  {
    size_t uid;
    size_t ouid;
    size_t i;
    for (uid = ouid = i = 0; i < mbox->messages_count; i++)
      {
        mum = mbox->umessages[i];
        uid = mum->uid;
        if (uid <= ouid)
          {
            uid = ouid + 1;
            mum->uid = ouid = uid;
            /* Note that modification for when expunging.  */
            mum->attr_flags |= MU_ATTRIBUTE_MODIFIED;
          }
        else
          ouid = uid;
      }
    if (mbox->messages_count > 0 && uid >= mbox->uidnext)
      {
        mum = mbox->umessages[0];
        mbox->uidnext = uid + 1;
        mum->attr_flags |= MU_ATTRIBUTE_MODIFIED;
      }
  }
  return status;
}
