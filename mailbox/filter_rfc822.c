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

#include <stdlib.h>
#include <errno.h>

#include <mailutils/property.h>
#include <filter0.h>

static int rfc822_property __P ((property_t, const char *, const char *));
static int rfc822_init __P ((filter_t));
static void rfc822_destroy __P ((filter_t));
static int rfc822_read __P ((filter_t, char *, size_t, off_t, size_t *));
static int rfc822_readline __P ((filter_t, char *, size_t, off_t, size_t *));
static int rfc822_read0 __P ((filter_t, char *, size_t, off_t, size_t *, int));

struct rfc822
{
  off_t r_offset; /* rfc822 offset.  */
  off_t s_offset; /* stream offset.  */
  size_t lines;
  int residue;
};

static struct _filter_record _rfc822_filter =
{
  "RFC822",
  rfc822_init,
  NULL,
  NULL,
  NULL,
};

/* Exported.  */
filter_record_t rfc822_filter = &_rfc822_filter;

static int
rfc822_property (property_t property, const char *key, const char *value)
{
  filter_t filter = property_get_owner (property);
  struct rfc822 *rfc822 = filter->data;
  (void)key;
  rfc822->lines = strtoul (value, NULL, 10);
  return 0;
}

static int
rfc822_init (filter_t filter)
{
  property_t property;
  int status;
  filter->data = calloc (1, sizeof (struct rfc822));
  if (filter->data == NULL)
    return ENOMEM;

  filter->_read = rfc822_read;
  filter->_readline = rfc822_readline;
  filter->_destroy = rfc822_destroy;

  /* We are interested in this property.  */
  if ((status = stream_get_property (filter->filter_stream, &property) != 0)
      || (status = property_add_defaults (property, "LINES", "0",
					  rfc822_property, NULL, filter)) != 0)
    {
      free (filter->data);
      filter->data = NULL;
      return status;
    }
  return 0;
}

static void
rfc822_destroy (filter_t filter)
{
  if (filter->data)
    free (filter->data);
}

static int
rfc822_read (filter_t filter, char *buffer, size_t buflen,
	      off_t off, size_t *pnread)
{
  return rfc822_read0 (filter, buffer, buflen, off, pnread, 0);
}

static int
rfc822_readline (filter_t filter, char *buffer, size_t buflen,
		 off_t off, size_t *pnread)
{
  return rfc822_read0 (filter, buffer, buflen, off, pnread, 1);
}

/* RFC 822 converter "\n" --> "\r\n"
   We maintain to offset, the rfc822 offset (r_offset) and the offset of
   the stream (s_offset).  If they do not match we go back as for as possible
   and start to read by 1 'till we reach the current offset.  */

static int
rfc822_read0 (filter_t filter, char *buffer, size_t buflen,
	      off_t off, size_t *pnread, int isreadline)
{
  size_t total = 0;
  int status = 0;
  struct rfc822 *rfc822 = filter->data;

  /* Catch up i.e bring us to the current offset.  */
  if (rfc822->r_offset != off)
    {
      rfc822->r_offset = off - rfc822->lines;
      rfc822->residue = 0;

      if (rfc822->r_offset < 0)
	rfc822->r_offset = 0;
      rfc822->s_offset = rfc822->r_offset;

      while (rfc822->r_offset < off)
	{
	  char c;
	  size_t n = 0;
	  status = stream_read (filter->stream, &c, 1, rfc822->s_offset, &n);
	  if (status != 0)
	    return status;
	  if (n == 0)
	    break;
	  if (c == '\n')
	    {
	      rfc822->r_offset++;
	      if (rfc822->r_offset == off)
		{
		  rfc822->residue = 1;
		  break;
		}
	    }
	  rfc822->r_offset++;
	  rfc822->s_offset++;
	}
    }

  do
    {
      size_t nread = 0;
      status = stream_readline (filter->stream, buffer, buflen,
				rfc822->s_offset, &nread);
      if (status != 0)
	return status;
      if (nread == 0)
	break;
      rfc822->r_offset += nread;
      rfc822->s_offset += nread;
      total += nread;
      buflen -= nread;
      if (buffer[nread - 1] == '\n')
	{
	  if (!rfc822->residue)
	    {
	      buffer[nread - 1] = '\r';
	      if (buflen == 0)
		{
		  rfc822->residue = 1;
		  break;
		}
	      buffer[nread] = '\n';
	      buflen--;
	      nread++;
	      total++;
	      rfc822->r_offset++;
	    }
	  else
	    rfc822->residue = 0;
	}
      buffer += nread;
    } while (buflen > 0 || !isreadline);

  if (isreadline)
    *buffer = '\0';

  if (pnread)
    *pnread = total;
  return status;
}
