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

static int header_parse (header_t *h, const char *blurb, size_t len);
/*
static int rfc822_set_value (header_t h, const char *fn, const char *fb,
			     size_t n, int replace);
static int rfc822_get_value (header_t h, const char *fn, char *fb,
			     size_t len, size_t *n);
static int rfc822_entry_count (header_t, size_t *num);
static int rfc822_entry_name (header_t h, size_t num, char *buf,
			      size_t buflen, size_t *total);
static int rfc822_entry_value (header_t h, size_t num, char *buf,
			       size_t buflen, size_t *total);
static int rfc822_get_istream (header_t h, istream_t *pis);
static int rfc822_get_ostream (header_t h, ostream_t *pos);
*/
static int header_read (istream_t is, char *buf, size_t buflen,
			off_t off, ssize_t *pnread);

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
  istream_t is;
  ostream_t os;

  size_t num;

  /* owner ? */
  void *owner;
  int ref_count;
};



int
header_init (header_t *ph, const char *blurb, size_t len, void *owner)
{
  header_t h;
  int status;
  h = calloc (1, sizeof (*h));
  if (h == NULL)
    return ENOMEM;
  h->owner = owner;

  status = header_parse (&h, blurb, len);
  if (status != 0)
    free (h);
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
	  istream_destroy (&(h->is), owner);
	  ostream_destroy (&(h->os), owner);

	  free (h->hdr);
	  free (h->blurb);
	  free (h);
	}
      *ph = NULL;
    }
}

/*
 * Parsing is done in a rather simple fashion.
 * meaning we just  consider an entry to be
 * a field-name an a field-value.  So they
 * maybe duplicate of field-name like "Received"
 * they are just put in the array, see _get_value()
 * on how to handle the case.
 */
static int
header_parse (header_t *h, const char *blurb, size_t len)
{
  header_t header;
  char *header_end;
  char *header_start;
  char *header_start2;
  struct _hdr *hdr;

  if (h == NULL || blurb == NULL || len == 0)
    return EINVAL;

  header = calloc (1, sizeof (*header));
  if (header == NULL)
    return ENOMEM;

  header->blurb = calloc (1, len);
  if (header->blurb == NULL)
    {
      free (header);
      return ENOMEM;
    }
  header->blurb_len = len;
  memcpy (header->blurb, blurb, len);

  for (header_start = header->blurb;; header_start = ++header_end)
    {
      /* get a header, a header is :
       * field-name ':'  ' ' field-value '\r' '\n'
       * [ (' ' | '\t') field-value '\r' '\n' ]
       */
      for (header_start2 = header_start;;header_start2 = ++header_end)
	{
	  header_end = memchr (header_start2, '\n', len);
	  if (header_end == NULL)
	    break;
	  else
	    {
	      len -= (header_end - header_start2 + 1);
	      if (len == 0)
		{
		  header_end = NULL;
		  break;
		}
	      if (header_end[1] != ' '
		  && header_end[1] != '\t')
		break; /* new header break the inner for */
	    }
	  /* *header_end = ' ';  smash LF */
	}

      if (header_end == NULL)
	break; /* bail out */

      hdr = realloc (header->hdr, (header->hdr_count + 1) * sizeof (*hdr));
      if (hdr == NULL)
	{
	  free (header->blurb);
	  free (header->hdr);
	  free (header);
	  return ENOMEM;
	}
      header->hdr = hdr;
      header->hdr_count++;
      /* Treats unix "From " specially */
      if ((header_end - header_start >= 5)
	  && strncmp (header_start, "From ", 5) == 0)
	{
	  header->hdr[header->hdr_count - 1].fn = header_start;
	  header->hdr[header->hdr_count - 1].fn_end = header_start + 6;
	  header->hdr[header->hdr_count - 1].fv = header_start + 6;
	  header->hdr[header->hdr_count - 1].fv_end = header_end;
	}
      else
	{
	  char *colon = memchr (header_start, ':', header_end - header_start);
	  if (colon == NULL)
	    {
	      /* Houston we have a problem */
	      free (header->blurb);
	      free (header->hdr);
	      free (header);
	      return EINVAL;
	    }
	  header->hdr[header->hdr_count - 1].fn = header_start;
	  header->hdr[header->hdr_count - 1].fn_end = colon;
	  /* skip leading spaces */
	  while (*(++colon) == ' ');
	  header->hdr[header->hdr_count - 1].fv = colon;
	  header->hdr[header->hdr_count - 1].fv_end = header_end;
	}
    }
  *h = header;
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
		  size_t buflen, size_t *n)
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
      if (fn_len == name_len && memcmp (header->hdr[i].fn, name, fn_len) == 0)
	{
	  fv_len = (header->hdr[i].fv_end - header->hdr[i].fv);
	  total += fv_len;
	  /* can everything fit in the buffer */
	  if (buffer && threshold > 0)
	    {
	      threshold -= fv_len;
	      if (threshold > 0)
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
  if (n)
    *n = total;
  return 0;
}

int
header_entry_count (header_t header, size_t *pnum)
{
  if (header == NULL)
      return EINVAL;
  if (pnum)
    *pnum = header->hdr_count;
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
header_read (istream_t is, char *buf, size_t buflen,
	     off_t off, ssize_t *pnread)
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
header_get_istream (header_t header, istream_t *pis)
{
  int err;
  if (header == NULL || pis == NULL)
    return EINVAL;
  /* already done */
  if  (header->is)
    *pis = header->is;

  err = istream_init (&(header->is), header_read, header->owner);
  if (err != 0)
    return err;
  *pis = header->is;
  return 0;
}

int
rfc822_get_ostream (header_t header, ostream_t *pos)
{
  if (header == NULL || pos == NULL)
    return EINVAL;
  return ENOSYS;
}
