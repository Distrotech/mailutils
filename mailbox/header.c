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
# include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <mailutils/stream.h>
#include <header0.h>

static int header_parse (header_t h, const char *blurb, int len);
static int header_read (stream_t is, char *buf, size_t buflen,
			off_t off, size_t *pnread);
static int header_write (stream_t os, const char *buf, size_t buflen,
			 off_t off, size_t *pnwrite);
static int fill_blurb (header_t header);

int
header_create (header_t *ph, const char *blurb, size_t len, void *owner)
{
  header_t h;
  int status = 0;
  h = calloc (1, sizeof (*h));
  if (h == NULL)
    return ENOMEM;
  h->owner = owner;

  /* Ignore the return value.  */
  status = header_parse (h, blurb, len);

  *ph = h;
  return status;
}

void
header_destroy (header_t *ph, void *owner)
{
  if (ph && *ph)
    {
      header_t h = *ph;

      /* Can we destroy ?.  */
      if (h->owner == owner)
	{
	  stream_destroy (&(h->stream), h);
	  if (h->hdr)
	    free (h->hdr);
	  if (h->blurb)
	    free (h->blurb);
	  free (h);
	}
      *ph = NULL;
    }
}

void *
header_get_owner (header_t header)
{
  return (header) ? header->owner : NULL;
}

/* Parsing is done in a rather simple fashion.
   meaning we just consider an entry to be
   a field-name an a field-value.  So they
   maybe duplicate of field-name like "Received"
   they are just put in the array, see _get_value()
   on how to handle the case.
   in the case of error .i.e a bad header construct
   we do a full stop and return what we have so far.  */
static int
header_parse (header_t header, const char *blurb, int len)
{
  char *header_end;
  char *header_start;
  char *header_start2;
  struct _hdr *hdr;

  /* Nothing to parse.  */
  if (blurb == NULL || len == 0)
    return 0;

  header->blurb_len = len;
  header->blurb = calloc (header->blurb_len + 1, sizeof(char));
  if (header->blurb == NULL)
    return ENOMEM;
  memcpy (header->blurb, blurb, header->blurb_len);

  if (header->hdr)
    free (header->hdr);
  header->hdr = NULL;
  header->hdr_count = 0;

  /* Get a header, a header is :
     field-name ':'  ' ' field-value '\r' '\n'
     [ (' ' | '\t') field-value '\r' '\n' ]
  */
  /* First loop goes throught the blurb */
  for (header_start = header->blurb;; header_start = ++header_end)
    {
      char *fn, *fn_end, *fv, *fv_end;

      if (header_start[0] == ' ' ||
	  header_start[0] == '\t' ||
	  header_start[0]== '\n')
	break;

      /* Second loop extract one header field.  */
      for (header_start2 = header_start;;header_start2 = ++header_end)
	{
	  header_end = memchr (header_start2, '\n', len);
	  if (header_end == NULL)
	    break;
	  else
	    {
	      len -= (header_end - header_start2 + 1);
	      if (len < 0)
		{
		  header_end = NULL;
		  break;
		}
	      if (header_end[1] != ' '
		  && header_end[1] != '\t')
		break; /* New header break the inner for. */
	    }
	  /* *header_end = ' ';  smash LF ? NO */
	}

      if (header_end == NULL)
	break; /* Bail out.  */

      /* Now save the header in the data structure.  */

      /* Treats unix "From " specially.  */
      if ((header_end - header_start >= 5)
	  && strncmp (header_start, "From ", 5) == 0)
	{
	  fn = header_start;
	  fn_end = header_start + 5;
	  fv = header_start + 5;
	  fv_end = header_end;
	}
      else /* Break the header in key: value */
	{
	  char *colon = memchr (header_start, ':', header_end - header_start);

	  /* Houston we have a problem.  */
	  if (colon == NULL)
	    break; /* Disregard the rest and bailout.  */

	  fn = header_start;
	  fn_end = colon;
	  /* Skip leading spaces. */
	  while (*(++colon) == ' ');
	  fv = colon;
	  fv_end = header_end;
	}

      /* Allocate a new slot for the field:value.  */
      hdr = realloc (header->hdr, (header->hdr_count + 1) * sizeof (*hdr));
      if (hdr == NULL)
	{
	  free (header->blurb);
	  free (header->hdr);
	  return ENOMEM;
	}
      header->hdr = hdr;
      header->hdr[header->hdr_count].fn = fn;
      header->hdr[header->hdr_count].fn_end = fn_end;
      header->hdr[header->hdr_count].fv = fv;
      header->hdr[header->hdr_count].fv_end = fv_end;
      header->hdr_count++;
    } /* for (header_start ...) */

 return 0;
}

/* FIXME: grossly inneficient, to many copies and reallocating.
   This all header business need a good rewrite.  */
int
header_set_value (header_t header, const char *fn, const char *fv, int replace)
{
  char *blurb;
  size_t len;

  if (header == NULL || fn == NULL || fv == NULL)
    return EINVAL;

  /* Overload.  */
  if (header->_set_value)
    return header->_set_value (header, fn, fv, replace);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  /* Easy approach: if replace, overwrite the field-{name,value} and readjust
     the pointers by calling header_parse () this is wastefull, we're just
     fragmenting the memory it can be done better.  But that may imply a
     rewite of the headers So for another day.  */
  if (replace)
    {
      size_t name_len;
      size_t i;
      size_t fn_len;
      size_t fv_len;
      len = header->blurb_len;
      for (name_len = strlen (fn), i = 0; i < header->hdr_count; i++)
	{
	  fn_len = header->hdr[i].fn_end - header->hdr[i].fn;
	  fv_len = header->hdr[i].fv_end - header->hdr[i].fv;
	  if (fn_len == name_len &&
	      strncasecmp (header->hdr[i].fn, fn, fn_len) == 0)
	    {
	      blurb = header->blurb;
	      if ((i + 1) < header->hdr_count)
		{
		  memmove (header->hdr[i].fn, header->hdr[i + 1].fn,
			   header->hdr[header->hdr_count - 1].fv_end
			   - header->hdr[i + 1].fn + 3);
		}
	      else
		{
		  header->hdr[i].fn[0] = '\n';
		  header->hdr[i].fn[1] = '\0';
		}
	      /* readjust the pointers if move */
	      len -= fn_len + fv_len + 3; /* :<sp>\n */
	      i--;
	      blurb = header->blurb;
	      header_parse (header, blurb, len);
	      free (blurb);
	    }
	}
    }

  /* Replacing was taking care of above now just add to the end the new
     header.  Really not cute.
     COLON SPACE NL =  3 ;  */
  len = strlen (fn) + strlen (fv) + 3;
  /* Add one for the NULL and leak a bit by adding one more
     it will be the separtor \n from the body if the first
     blurb did not have it.  */
  blurb = calloc (header->blurb_len + len + 2, 1);
  if (blurb == NULL)
    return ENOMEM;

  sprintf (blurb, "%s: %s", fn, fv);

  /* Strip off trailing newlines.  */
  while (blurb[strlen (blurb) - 1] == '\n')
    {
      blurb[strlen (blurb) - 1] = '\0';
    }
  len = strlen (blurb);
  blurb[len] = '\n';
  len++;

  /* Prepend the rest of the headers.  */
  if (header->blurb)
    {
      memcpy (blurb + len, header->blurb, header->blurb_len);
      free (header->blurb);
    }
  else
    blurb [len] = '\n';

  /* before parsing the new blurb make sure it is properly terminated
     by \n\n. The trailing NL separator.  */
  if (blurb[header->blurb_len + len - 1] != '\n'
      || blurb[header->blurb_len + len - 2] != '\n')
    {
      blurb[header->blurb_len + len] = '\n';
      len++;
    }
  header_parse (header, blurb, len + header->blurb_len);
  free (blurb);
  return 0;
}

int
header_get_value (header_t header, const char *name, char *buffer,
		  size_t buflen, size_t *pn)
{
  size_t i = 0;
  size_t name_len;
  size_t total = 0, fn_len = 0, fv_len = 0;
  int threshold;
  int err = 0;

  if (header == NULL || name == NULL)
    return EINVAL;

  /* Overload.  */
  if (header->_get_value)
    return header->_get_value (header, name, buffer, buflen, pn);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  /* We set the threshold to be 1 less for the null.  */
  threshold = --buflen;

  /* Caution: We may have more then one value for a field name, for example
     a "Received" field-name is added by each passing MTA.  The way that the
     parsing (_parse()) is done it's not take to account.  So we just stuff
     in the buffer all the field-values to a corresponding field-name.
     FIXME: Should we kosher the output ? meaning replace occurences of
     " \t\r\n" for spaces ? for now we don't.  */
  for (name_len = strlen (name), i = 0; i < header->hdr_count; i++)
    {
      fn_len = header->hdr[i].fn_end - header->hdr[i].fn;
      if (fn_len == name_len &&
	  strncasecmp (header->hdr[i].fn, name, fn_len) == 0)
	{
	  fv_len = (header->hdr[i].fv_end - header->hdr[i].fv);
	  total += fv_len;
	  /* Can everything fit in the buffer.  */
	  if (buffer && threshold > 0)
	    {
	      threshold -= fv_len;
	      if (threshold >= 0)
		{
		  memcpy (buffer, header->hdr[i].fv, fv_len);
		  buffer += fv_len;
		}
	      else if (threshold < 0)
		{
		  threshold += fv_len;
		  memcpy (buffer, header->hdr[i].fv, threshold);
		  buffer += threshold;
		  threshold = 0;
		}
	    }
	}
    }
  if (buffer)
    *buffer = '\0'; /* Null terminated.  */
  if (pn)
    *pn = total;

  if (total == 0)
    err = ENOENT;

  return err;
}

int
header_set_lines (header_t header, int (*_lines)
		 (header_t, size_t *), void *owner)
{
  if (header ==  NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_lines = _lines;
  return 0;
}

int
header_lines (header_t header, size_t *plines)
{
  int n;
  size_t lines = 0;
  if (header == NULL || plines == NULL)
    return EINVAL;

  /* Overload.  */
  if (header->_lines)
    return header->_lines (header, plines);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  for (n = header->blurb_len - 1; n >= 0; n--)
    {
      if (header->blurb[n] == '\n')
	lines++;
    }
  if (plines)
    *plines = lines;
  return 0;
}

int
header_set_size (header_t header, int (*_size)
		 (header_t, size_t *), void *owner)
{
  if (header ==  NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_size = _size;
  return 0;
}

int
header_size (header_t header, size_t *psize)
{
  if (header == NULL)
      return EINVAL;

  /* Overload.  */
  if (header->_size)
    return header->_size (header, psize);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (psize)
    *psize = header->blurb_len;
  return 0;
}

int
header_set_get_value (header_t header, int (*_get_value)
		     (header_t, const char *, char *, size_t, size_t *),
		     void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_get_value = _get_value;
  return 0;
}

int
header_set_set_value (header_t header, int (*_set_value)
		      (header_t , const char *, const char *, int),
		      void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_set_value = _set_value;
  return 0;
}

int
header_set_stream (header_t header, stream_t stream, void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->stream = stream;
  return 0;
}

static int
fill_blurb (header_t header)
{
  stream_t is;
  int status;
  char buf[1024];
  char *tbuf;
  size_t nread = 0;

  status = header_get_stream (header, &is);
  if (status != 0)
    return status;

  do
    {
      status = stream_read (is, buf, sizeof (buf), header->temp_blurb_len,
			    &nread);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      free (header->temp_blurb);
	      header->temp_blurb = NULL;
	      header->temp_blurb_len = 0;
	    }
	  return status;
	}
      if (nread > 0)
	{
	  tbuf = realloc (header->temp_blurb, header->temp_blurb_len + nread);
	  if (tbuf == NULL)
	    {
	      free (header->temp_blurb);
	      header->temp_blurb = NULL;
	      header->temp_blurb_len = 0;
	      return ENOMEM;
	    }
	  header->temp_blurb = tbuf;
	  memcpy (header->temp_blurb + header->temp_blurb_len, buf, nread);
	  header->temp_blurb_len += nread;
	}
    }
  while (nread > 0);

  /* parse it. */
  status = header_parse (header, header->temp_blurb, header->temp_blurb_len);
  if (header->temp_blurb)
    {
      free (header->temp_blurb);
      header->temp_blurb = NULL;
    }
  header->temp_blurb_len = 0;
  return status;
}

static int
header_write (stream_t os, const char *buf, size_t buflen,
	      off_t off, size_t *pnwrite)
{
  header_t header = stream_get_owner (os);
  if (os == NULL || header == NULL)
    return EINVAL;

  (void)buf; (void)off;
  if (buflen == 0)
    return 0;

  if (pnwrite)
    *pnwrite = 0;

  return ENOSYS;
}

static int
header_read (stream_t is, char *buf, size_t buflen,
	     off_t off, size_t *pnread)
{
  header_t header = stream_get_owner (is);
  int len;

  if (is == NULL || header == NULL)
    return EINVAL;

  len = header->blurb_len - off;
  if (len > 0)
    {
      if (buf)
	{
	  len = (buflen < (size_t)len) ? buflen : len;
	  memcpy (buf, header->blurb + off, len);
	}
    }
  else
    len = 0;

  if (pnread)
    *pnread = len;
  return 0;
}

int
header_get_stream (header_t header, stream_t *pstream)
{
  if (header == NULL || pstream == NULL)
    return EINVAL;
  if (header->stream == NULL)
    {
      int status = stream_create (&(header->stream), MU_STREAM_RDWR, header);
      if (status != 0)
	return status;
      stream_set_read  (header->stream, header_read, header);
      stream_set_write (header->stream, header_write, header);
    }
  *pstream = header->stream;
  return 0;
}
