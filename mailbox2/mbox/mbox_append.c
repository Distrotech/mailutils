/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

#define IS_X_IMAPBASE(buf) (\
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

#define IS_X_UID(buf) (\
(buf[0] == 'X' || buf[0] == 'x') \
 && (buf[1] == '-') \
 && (buf[2] == 'U' || buf[2] == 'u') \
 && (buf[3] == 'I' || buf[3] == 'i') \
 && (buf[4] == 'D' || buf[4] == 'd') \
 && (buf[5] == ':' || buf[5] == ' ' || buf[5] == '\t'))

#define IS_CONTENT_LENGTH(buf) (\
(buf[0] == 'C' || buf[0] == 'c') && \
(buf[1] == 'O' || buf[1] == 'o') && \
(buf[2] == 'N' || buf[2] == 'n') && \
(buf[3] == 'T' || buf[3] == 't') && \
(buf[4] == 'E' || buf[4] == 'e') && \
(buf[5] == 'N' || buf[5] == 'n') && \
(buf[6] == 'T' || buf[6] == 't') && \
(buf[7] == '-') && \
(buf[8] == 'L' || buf[8] == 'l') && \
(buf[9] == 'E' || buf[9] == 'e') && \
(buf[10] == 'N' || buf[10] == 'n') && \
(buf[11] == 'G' || buf[11] == 'g') && \
(buf[12] == 'T' || buf[12] == 't') && \
(buf[13] == 'H' || buf[13] == 'h') && \
(buf[14] == ':' || buf[14] == ' ' || buf[14] == '\t'))

#define IS_STATUS(buf) (\
(buf[0] == 'S' || buf[0] == 's') && \
(buf[1] == 'T' || buf[1] == 't') && \
(buf[2] == 'A' || buf[2] == 'a') && \
(buf[3] == 'T' || buf[3] == 't') && \
(buf[4] == 'U' || buf[4] == 'u') && \
(buf[5] == 'S' || buf[5] == 's') && \
(buf[6] == ':' || buf[6] == ' ' || buf[6] == '\t'))

#define IS_FROM_(buf) (\
(buf[0] == 'F' || buf[0] == 'f') && \
(buf[1] == 'R' || buf[1] == 'r') && \
(buf[2] == 'O' || buf[2] == 'o') && \
(buf[3] == 'M' || buf[3] == 'm') && \
(buf[4] == ' ' || buf[4] == '\t'))

/* Save the uidvalidity:
   + if it is the first message append
   and save_uidvalidity is set.

   - strip X-IMAPBASE, X-UID, Content-Length and Status.

   - add X-UID base on mbox->uidnext.

   - mangle any leading "From " in the body to ">From "

   - update the size of the mailbox.

   - Refuse to append if the mailbox is change on disk.
*/

/* Assuming that the file is lock:
   Add the unix separtor the form is:
   From user cdate_format\n
*/
static int
mbox_append_separator (mbox_t mbox, const char *sep)
{
  char separator[256];
  size_t len;
  const char nl = '\n';
  int status = 0;

  if (sep == NULL)
    {
      time_t now;
      struct tm *ptm;

      now = time (NULL);
      ptm = gmtime (&now);
      len = strftime (separator, sizeof separator,
		      "From unknown %a %b %d %H:%M:%S %Y\n", ptm);
      if (len == 0)
	{
	  len = snprintf (separator, sizeof separator,
			  "From unknown %s", ctime (&now));
	}
      sep = separator;
    }
  else
    len = strlen (sep);

  /* Write the separator.  */
  status = stream_write (mbox->carrier, sep, len, mbox->woffset, &len);
  if (status != 0)
    return status;
  mbox->woffset += len;

  /* Add the trailing newline.  */
  if (len && sep[len - 1] != '\n')
    {
      status = stream_write (mbox->carrier, &nl, 1, mbox->woffset, &len);
      if (status != 0)
	return status;
      mbox->woffset += len;
    }
  return status;
}

/* Assuming that the file is lock.
   Strip away Content-Length in the header add add the separating
   newline between the header and the body.
 */
static int
mbox_append_header (mbox_t mbox, attribute_t attribute, stream_t hstream,
		    int save_uidvalidity, unsigned long uid)
{
  char buf[1024];
  size_t n = 0;
  size_t nread = 0;
  int status = 0;
  const char nl = '\n';

  do
    {
      status = stream_readline (hstream, buf, sizeof buf, mbox->roffset, &nread);
      if (status != 0)
	return status;

      mbox->roffset += nread;

      /* A newline means the start of the body.  */
      if (*buf == '\n')
	break;

      /* Skip X-IMAPBase it has special meaning for us.  */
      /* Skip X-UID. A new one will be provided.  */
      /* Skip Status, use the attribute.  */
      /* Skip Content-Length, too often bad.  */
      if (IS_X_IMAPBASE (buf) || IS_X_UID (buf)
	  || IS_STATUS (buf) || IS_CONTENT_LENGTH (buf))
	continue;

      status = stream_write (mbox->carrier, buf, nread, mbox->woffset, &n);
      if (status != 0)
	return status;
      mbox->woffset += n;
    }
  while (nread > 0);

  /* Rewrite the X-IMAPbase, if necesary, and only for the first msg.  */
  if (mbox->uidnext < 2 && save_uidvalidity)
    {
      n = snprintf (buf, sizeof buf, "X-IMAPbase: %lu %lu\n",
		    mbox->uidvalidity, mbox->uidnext);
      status = stream_write (mbox->carrier, buf, n, mbox->woffset, &n);
      if (status != 0)
	return status;
      mbox->woffset += n;
    }

  /* Rewrite the  Status for the attribute.  */
  if (attribute)
    {
      mbox_attribute_to_status (attribute, buf, sizeof buf, &n);
      status = stream_write (mbox->carrier, buf, n, mbox->woffset, &n);
      if (status != 0)
	return status;
      mbox->woffset += n;
    }

  /* Rewrite the X-UID marker . */
  n = snprintf (buf, sizeof buf, "X-UID: %lu\n", (uid) ? uid : mbox->uidnext);
  status = stream_write (mbox->carrier, buf, n, mbox->woffset, &n);
  if (status != 0)
    return status;
  mbox->woffset += n;

  /* New line separator of the Header.  */
  status = stream_write (mbox->carrier, &nl , 1, mbox->woffset, &n);
  if (status != 0)
    return status;
  mbox->woffset += n;
  return status;
}

/* Assuming that the file is lock.
   Do the mangling, line startin with "From " is mangle to ">From "
 */
static int
mbox_append_body (mbox_t mbox, stream_t bstream)
{
  char buffer[1024];
  char *buf;
  int was_complete_line;
  size_t nread = 0;
  const char nl = '\n';
  int status;
  size_t n = 0;

  /* For "From " mangling.  */
  *buffer = '>';
  was_complete_line = 1; /* Say we start as complete line.  */
  do
    {
      buf = buffer + 1;
      status = stream_readline (bstream, buf, sizeof (buffer) - 1,
				mbox->roffset, &nread);
      if (status != 0)
	return status;
      mbox->roffset += nread;

      /* Unix Mbox:
	 Since it's possibpe for a message to contain lines that looks
	 like message separators, special care must be taken when adding
	 a message to an mbox folder this is done by prepending a '>'
	 character to any lines starting with zero or more '>' characters
	 folowed by "From "
	 p436 "Internet Email Protocols  a Debeloper's Guid"
	 Kevin Johnson.
      */
      if (was_complete_line)
	{
	  char *s = buf;
	  /* Eat all the '>'.  */
	  while (*s == '>')
	    s++;

	  if (IS_FROM_ (s))
	    {
	      buf = buffer;
	      nread++;
	    }
	}
      status = stream_write (mbox->carrier, buf, nread, mbox->woffset, &n);
      if (status != 0)
	return status ;
      mbox->woffset += n;

      /* Register if we read a complete line.  */
      was_complete_line =  (nread && buf[nread - 1] == '\n') ? 1 : 0;
    }
  while (nread > 0);

  /* New line separator for the next message.  */
  status = stream_write (mbox->carrier, &nl, 1, mbox->woffset, &n);
  if (status != 0)
    return status;
  mbox->woffset += n;
  return status;
}

int
mbox_append_hb0 (mbox_t mbox, const char *sep, attribute_t attribute,
		 stream_t hstream, stream_t bstream,
		 int save_uidvalidity, unsigned long uid)
{
  int status = 0;

  mbox_debug_print (mbox, "append_hb0(uid=%lu)", uid);

  if (mbox == NULL || hstream == NULL || bstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  switch (mbox->state)
    {
    case MU_MBOX_NO_STATE:
      {
	unsigned long uidvalidity;
	unsigned long uidnext;

	/* Get the uidvalidity for this mbox.  */
	mbox_get_uidvalidity (mbox, &uidvalidity);
	mbox_get_uidnext (mbox, &uidnext);

	/* Grab the lock.  */
	status = lockfile_lock (mbox->lockfile);
	if (status != 0)
	  break;

	mbox->woffset = mbox->roffset = 0;
	/* Move to the end of the stream.  */
	status = stream_get_size (mbox->carrier, &mbox->woffset);
	if (status != 0)
	    break;
	mbox->state = MU_MBOX_STATE_APPEND_SEPARATOR;
      }

    case MU_MBOX_STATE_APPEND_SEPARATOR:
      {
	status = mbox_append_separator (mbox, sep);
	if (status != 0)
	  break;
	mbox->state = MU_MBOX_STATE_APPEND_HEADER;
      }

    case MU_MBOX_STATE_APPEND_HEADER:
      {
	status = mbox_append_header (mbox, attribute, hstream,
				     save_uidvalidity, uid);
	if (status != 0)
	  break;
        mbox->state = MU_MBOX_STATE_APPEND_BODY;
      }

    case MU_MBOX_STATE_APPEND_BODY:
      {
	/* Hack it the same stream. do not reset the mbox->roffset.  */
	if (hstream != bstream)
	  mbox->roffset = 0;
	status = mbox_append_body (mbox, bstream);
	if (status != 0)
	  break;
        mbox->state = MU_MBOX_NO_STATE;
      }

    default:
      mbox->woffset = mbox->roffset = 0;
      break;
    }

  /* Maintain the lock if EAGAIN.  */
  if (status != 0)
    {
      if (status != MU_ERROR_TRY_AGAIN)
	{
	  mbox->state = MU_MBOX_NO_STATE;
	  mbox->woffset = mbox->roffset = 0;
	  lockfile_unlock (mbox->lockfile);
	}
    }
  else
    {
      stream_flush (mbox->carrier);
      lockfile_unlock (mbox->lockfile);
      mbox->uidnext++;
    }

  return status;
}

int
mbox_append_hb (mbox_t mbox, const char *sep, attribute_t attribute,
		stream_t hstream, stream_t bstream)
{
  return mbox_append_hb0 (mbox, sep, attribute, hstream, bstream, 1, 0);
}

int
mbox_append (mbox_t mbox, const char *sep, attribute_t attribute,
	     stream_t stream)
{
  return mbox_append_hb (mbox, sep, attribute, stream, stream);
}
