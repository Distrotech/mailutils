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
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "bio.h"

/* This is so annoying, we can not use the standard I/O(stdio) because a
   socket may write/read less then requested and with the stdio functions
   there is no way to know how much data was written/read, meaning also things
   can get tricky in NONBLOCKING.  */

struct _bio
{
  char *read_buffer;  /* Read Buffer.  */
  char *read_ptr;
  size_t read_buflen;

  char *write_buffer;  /* Write Buffer.  */
  char *write_ptr;
  size_t write_buflen;

  stream_t stream;
};

int
bio_create (bio_t *pbio, stream_t stream)
{
  bio_t bio;
  /* 512 is the  minimum for POP answer buffer.  */
#define MIOBUF 512

  if (pbio == NULL)
    return EINVAL;

  bio = calloc (1, sizeof (*bio));
  if (bio == NULL)
    return ENOMEM;

  /* + 1 for the sentinel.  */
  bio->read_buffer = calloc (MIOBUF + 1, sizeof (char));
  if (bio->read_buffer == NULL)
    {
      bio_destroy (&bio);
      return ENOMEM;
    }
  bio->read_ptr = bio->read_buffer;
  bio->read_buflen = MIOBUF;

  /* + 1 for the sentinel.  */
  bio->write_buffer = calloc (MIOBUF/2 + 1, sizeof (char));
  if (bio->write_buffer == NULL)
    {
      bio_destroy (&bio);
      return ENOMEM;
    }
  bio->write_ptr = bio->write_buffer;
  bio->write_buflen = MIOBUF;

  bio->stream = stream;
  *pbio = bio;
  return 0;
}

void
bio_destroy (bio_t *pbio)
{
  if (pbio && *pbio)
    {
      free ((*pbio)->read_buffer);
      free ((*pbio)->write_buffer);
      /* We don't close the file descriptor ? */
      /* stream_close (stream); */
      free ((*pbio));
      *pbio = NULL;
    }
}

/* buffered read */
int
bio_read (bio_t bio, char *ptr, size_t n, size_t *pnread)
{
  int nleft;
  size_t nread = 0;
  size_t len = 0;

  /* sanity check */
  if (bio == NULL || pnread == NULL)
    return EINVAL;

  /* noop */
  if (n == 0 || ptr == NULL)
    {
      *pnread = 0;
      return 0;
    }

  /* fill up the global buffer */
  if (bio->read_ptr == bio->read_buffer)
    {
      int err = stream_read (bio->stream, bio->read_buffer,
			     bio->read_buflen, 0, &len);
      if (err != 0)
	return err;
      /* If len == read_buflen, read_ptr points to the sentinel.  */
      bio->read_ptr = bio->read_buffer + len;
    }
  else
    len = bio->read_ptr - bio->read_buffer;

  /* How much can wd copy out ?  */
  nleft = n - len;

  /* We got more then requested.  */
  if (nleft < 0)
    {
      size_t sentinel;
      nread = n;
      /* where to move the sentinel to mark the end.  */
      sentinel = bio->read_ptr - (bio->read_buffer + nread);
      memcpy (ptr, bio->read_buffer, nread);
      memmove (bio->read_buffer, bio->read_buffer + nread, sentinel);
      bio->read_ptr = bio->read_buffer + sentinel  ;
    }
  else
    {
      /* Drain our buffer.  */;
      nread = len;
      memcpy (ptr, bio->read_buffer, nread);
      bio->read_ptr = bio->read_buffer;
    }
  if (pnread)
    *pnread = nread;
  return 0;
}

int
bio_write (bio_t bio, const char *ptr, size_t n, size_t *pnwriten)
{
  int nleft;
  int err = 0;
  size_t nwriten = 0;
  size_t total = 0;

  nleft = n;

  /* First try to send it all.  */
  while (nleft > 0)
    {
      err = stream_write (bio->stream, ptr, nleft, 0, &nwriten);
      if (err != 0)
	break;
      nleft -= nwriten;
      total += nwriten;
      ptr += nwriten;
    }

  /* Not recoverable.  */
  if (err != EAGAIN && err != EINTR)
    bio->write_ptr = bio->write_buffer;

  if (pnwriten)
    *pnwriten = total;

  return err;
}

/* Return the number of char read excluding the null char */
int
bio_readline (bio_t bio, char *ptr, size_t maxlen, size_t *pwriten)
{
  size_t rc = 0;
  size_t n = 0;
  int err;
  char c;

  /* Noop.  */
  if (ptr == NULL || maxlen == 0)
    {
      if (pwriten)
	*pwriten = 0;
      return 0;
    }

  maxlen--;

  while ((err = bio_read (bio, &c, 1, &rc)) == 0 && rc == 1)
    {
      *ptr++ = c;
      n++;
      if (c == '\n')
	break;  /* newline is stored, like fgets () */
      if (n == maxlen)
	break;
    }
  /* This is tricky, We may have data in the buffer, so we flush it
     first and return success.  On the next read if the error persist
     it will be returned. This can happen for NONBLOCKING.  */
  if (n > 0)
    err = 0;

  *ptr = '\0';
  if (pwriten)
    *pwriten = n;
  //fprintf (stderr, "\n%d %d BIO %s\n", n, *(ptr -n), (ptr -n ));
  return err;
}
