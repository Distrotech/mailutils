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

#include <header.h>
#include <io0.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static int header_parse (header_t h, char *blurb, int len);
static int header_read (stream_t is, char *buf, size_t buflen,
			off_t off, size_t *pnread);
static int header_write (stream_t os, const char *buf, size_t buflen,
			 off_t off, size_t *pnwrite);

struct _hdr
{
  char *fn;
  char *fn_end;
  char *fv;
  char *fv_end;
};

struct _header
{
  /* Data */
  char *blurb;
  size_t blurb_len;
  size_t hdr_count;
  struct _hdr *hdr;

  /* streams */
  stream_t stream;

  /* owner ? */
  void *owner;
  int ref_count;
};

int
header_create (header_t *ph, const char *blurb, size_t len, void *owner)
{
  header_t h;
  int status;
  h = calloc (1, sizeof (*h));
  if (h == NULL)
    return ENOMEM;
  h->owner = owner;

  header_parse (h, (char *)blurb, len);

  status = stream_create (&(h->stream), MU_STREAM_READ|MU_STREAM_WRITE, h);
  if (status != 0)
    return status;

  stream_set_read  (h->stream, header_read, h);
  stream_set_write (h->stream, header_write, h);

  *ph = h;
  return status;
}

void
header_destroy (header_t *ph, void *owner)
{
  if (ph && *ph)
    {
      header_t h = *ph;

      /* if destroy is call always decremente */
      h->ref_count--;

      /* can we destroy ? */
      if ((h->owner && h->owner == owner) ||
	  (h->owner == NULL && h->ref_count <= 0))
	{
	  /* io */
	  stream_destroy (&(h->stream), h);

	  free (h->hdr);
	  free (h->blurb);
	  free (h);
	}
      *ph = NULL;
    }
}

/*
 * Parsing is done in a rather simple fashion.
 * meaning we just consider an entry to be
 * a field-name an a field-value.  So they
 * maybe duplicate of field-name like "Received"
 * they are just put in the array, see _get_value()
 * on how to handle the case.
 * in the case of error .i.e a bad header construct
 * we do a full stop and return what we have so far.
 */
static int
header_parse (header_t header, char *blurb, int len)
{
  char *header_end;
  char *header_start;
  char *header_start2;
  struct _hdr *hdr;

  /* nothing to parse */
  if (blurb == NULL || len == 0)
    return 0;

  header->blurb_len = len;
  header->blurb = calloc (1, header->blurb_len + 1);
  if (header->blurb == NULL)
    return ENOMEM;
  memcpy (header->blurb, blurb, header->blurb_len);

  for (header_start = header->blurb;; header_start = ++header_end)
    {
      char *fn, *fn_end, *fv, *fv_end;
      /* get a header, a header is :
       * field-name ':'  ' ' field-value '\r' '\n'
       * [ (' ' | '\t') field-value '\r' '\n' ]
       */

      if (header_start[0] == ' ' ||
	  header_start[0] == '\t' ||
	  header_start[0]== '\n')
	break;

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
		break; /* new header break the inner for */
	    }
	  /* *header_end = ' ';  smash LF ? NO */
	}

      if (header_end == NULL)
	break; /* bail out */

      /* Treats unix "From " specially */
      if ((header_end - header_start >= 5)
	  && strncmp (header_start, "From ", 5) == 0)
	{
	  fn = header_start;
	  fn_end = header_start + 5;
	  fv = header_start + 5;
	  fv_end = header_end;
	}
      else
	{
	  char *colon = memchr (header_start, ':', header_end - header_start);

	  /* Houston we have a problem */
	  if (colon == NULL)
	    break; /* disregard the rest and bailout */

	  fn = header_start;
	  fn_end = colon;
	  /* skip leading spaces */
	  while (*(++colon) == ' ');
	  fv = colon;
	  fv_end = header_end;
	}

      /* allocate a new slot for the field:value */
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

#if 0
  header->blurb_len -= len;
  if (header->blurb_len <= 0)
    {
      free (header->blurb);
      free (header->hdr);
      return EINVAL;
    }
  /* always add the separtor LF */
  header->blurb [header->blurb_len] = '\n';
  header->blurb_len++;
#endif
  return 0;
}

int
header_set_value (header_t h, const char *fn, const char *fv,
		  size_t n, int replace)
{
  (void)h; (void)fn; (void)fv; (void)n; (void)replace;
  return ENOSYS;
}

int
header_get_value (header_t header, const char *name, char *buffer,
		  size_t buflen, size_t *pn)
{
  size_t i = 0;
  size_t name_len;
  size_t total = 0, fn_len = 0, fv_len = 0;
  int threshold;

  if (header == NULL || name == NULL)
    return EINVAL;

  /* we set the threshold to be 1 less for the null */
  threshold = --buflen;

  /*
   * Caution: We may have more then one value for a field
   * name, for example a "Received" field-name is added by
   * each passing MTA.  The way that the parsing (_parse())
   * is done it's not take to account.  So we just stuff in
   * the buffer all the field-values to a corresponding field-name.
   * FIXME: Should we kosher the output ? meaning replace
   * occurences of " \t\r\n" for spaces ? for now we don't.
   */
  for (name_len = strlen (name), i = 0; i < header->hdr_count; i++)
    {
      fn_len = header->hdr[i].fn_end - header->hdr[i].fn;
      if (fn_len == name_len &&
	  strncasecmp (header->hdr[i].fn, name, fn_len) == 0)
	{
	  fv_len = (header->hdr[i].fv_end - header->hdr[i].fv);
	  total += fv_len;
	  /* can everything fit in the buffer */
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
    *buffer = '\0'; /* null terminated */
  if (pn)
    *pn = total;
  return 0;
}

int
header_entry_count (header_t header, size_t *pnum)
{
  if (header == NULL)
    {
      if (pnum)
	*pnum = 0;
      return EINVAL;
    }
  if (pnum)
    *pnum = header->hdr_count;
  return 0;
}

int
header_lines (header_t header, size_t *plines)
{
  int n;
  size_t t = 0;
  if (header == NULL)
    return EINVAL;
  for (n = header->blurb_len - 1; n >= 0; n--)
    {
      if (header->blurb[n] == '\n')
	t++;
    }
  if (plines)
    *plines = t;
  return 0;
}

int
header_size (header_t header, size_t *pnum)
{
  if (header == NULL)
      return EINVAL;
  if (pnum)
    *pnum = header->blurb_len;
  return 0;
}

int
header_entry_name (header_t header, size_t num, char *buf,
		   size_t buflen, size_t *nwritten)
{
  size_t len;
  if (header == NULL)
    return EINVAL;
  if (header->hdr_count == 0 || num > header->hdr_count)
    return ENOENT;
  len = header->hdr[num].fn_end - header->hdr[num].fn;
  /* save one for the null */
  --buflen;
  if (buf && buflen > 0)
    {
      buflen = (len > buflen) ? buflen : len;
      memcpy (buf, header->hdr[num].fn, buflen);
      buf[buflen] = '\0';
    }
  if (nwritten)
    *nwritten = len;
  return 0;
}

int
header_entry_value (header_t header, size_t num, char *buf,
		    size_t buflen, size_t *nwritten)
{
  size_t len;
  if (header == NULL)
    return EINVAL;
  if (header->hdr_count == 0 || num > header->hdr_count)
    return ENOENT;
  len = header->hdr[num].fv_end - header->hdr[num].fv;
  /* save one for the null */
  --buflen;
  if (buf && buflen > 0)
    {
      buflen = (len > buflen) ? buflen : len;
      memcpy (buf, header->hdr[num].fv, buflen);
      buf[buflen] = '\0';
    }
  if (nwritten)
    *nwritten = len;
  return 0;
}

static int
header_write (stream_t os, const char *buf, size_t buflen,
	      off_t off, size_t *pnwrite)
{
  header_t header;
  if (os == NULL || (header = (header_t)os->owner) == NULL)
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
  header_t header;
  ssize_t len;

  if (is == NULL || (header = (header_t)is->owner) == NULL)
    return EINVAL;

  len = header->blurb_len - off;
  if ((header->blurb_len - off) > 0)
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
  *pstream = header->stream;
  return 0;
}
