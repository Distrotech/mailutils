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

#include <header0.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static int rfc822_parse (header_t h, const char *blurb, size_t len);
static int rfc822_set_value (header_t h, const char *fn, const char *fb,
			     size_t n, int replace);
static int rfc822_get_value (header_t h, const char *fn, char *fb,
			     size_t len, size_t *n);
static int rfc822_entry_count (header_t, size_t *num);
static int rfc822_entry_name (header_t h, size_t num, char *buf,
			      size_t buflen, size_t *total);
static int rfc822_entry_value (header_t h, size_t num, char *buf,
			       size_t buflen, size_t *total);
static ssize_t rfc822_get_data (header_t h, char *buf, size_t buflen,
				off_t off, int *err);

struct _rfc822
{
  char *blurb;
  size_t blurb_len;
  size_t hdr_count;
  struct _hdr *hdr;
};

typedef struct _rfc822 * rfc822_t;


int
rfc822_init (header_t *ph, const char *blurb, size_t len)
{
  header_t h;
  int status;
  h = calloc (1, sizeof (*h));
  if (h == NULL)
    return ENOMEM;
  h->_init = rfc822_init;
  h->_destroy = rfc822_destroy;
  h->_parse = rfc822_parse;
  h->_get_value = rfc822_get_value;
  h->_set_value = rfc822_set_value;
  h->_entry_count = rfc822_entry_count;
  h->_entry_name = rfc822_entry_name;
  h->_entry_value = rfc822_entry_value;
  h->_get_data = rfc822_get_data;

  status = h->_parse (h, blurb, len);
  if (status != 0)
    free (h);
  *ph = h;
  return status;
}

void
rfc822_destroy (header_t *ph)
{
  if (ph && *ph)
    {
      header_t h = *ph;
      /* own by a message */
      if (h->message)
	{
	  *ph = NULL;
	  return;
	}
      if (h->data)
	{
	  rfc822_t rfc = (rfc822_t)h->data;
	  free (rfc->hdr);
	  free (rfc->blurb);
	  free (rfc);
	}
      free (h);
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
rfc822_parse (header_t h, const char *blurb, size_t len)
{
  rfc822_t rfc;
  char *header_end;
  char *header_start;
  char *header_start2;
  struct _hdr *hdr;

  if (h == NULL || blurb == NULL || len == 0)
    return EINVAL;

  rfc = calloc (1, sizeof (*rfc));
  if (rfc == NULL)
    return ENOMEM;

  rfc->blurb = calloc (1, len);
  if (rfc->blurb == NULL)
    {
      free (rfc);
      return ENOMEM;
    }
  rfc->blurb_len = len;
  memcpy (rfc->blurb, blurb, len);

  for (header_start = rfc->blurb;; header_start = ++header_end)
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

      hdr = realloc (rfc->hdr, (rfc->hdr_count + 1) * sizeof (*hdr));
      if (hdr == NULL)
	{
	  free (rfc->blurb);
	  free (rfc->hdr);
	  free (rfc);
	  return ENOMEM;
	}
      rfc->hdr = hdr;
      rfc->hdr_count++;
      /* Treats unix "From " specially */
      if ((header_end - header_start >= 5)
	  && strncmp (header_start, "From ", 5) == 0)
	{
	  rfc->hdr[rfc->hdr_count - 1].fn = header_start;
	  rfc->hdr[rfc->hdr_count - 1].fn_end = header_start + 6;
	  rfc->hdr[rfc->hdr_count - 1].fv = header_start + 6;
	  rfc->hdr[rfc->hdr_count - 1].fv_end = header_end;
	}
      else
	{
	  char *colon = memchr (header_start, ':', header_end - header_start);
	  if (colon == NULL)
	    {
	      /* Houston we have a problem */
	      free (rfc->blurb);
	      free (rfc->hdr);
	      free (rfc);
	      return EINVAL;
	    }
	  rfc->hdr[rfc->hdr_count - 1].fn = header_start;
	  rfc->hdr[rfc->hdr_count - 1].fn_end = colon;
	  /* skip leading spaces */
	  while (*(++colon) == ' ');
	  rfc->hdr[rfc->hdr_count - 1].fv = colon;
	  rfc->hdr[rfc->hdr_count - 1].fv_end = header_end;
	}
    }
  h->data = rfc;
  return 0;
}

static int
rfc822_set_value (header_t h, const char *fn, const char *fv,
		  size_t n, int replace)
{
  (void)h; (void)fn; (void)fv; (void)n; (void)replace;
  return ENOSYS;
}

static int
rfc822_get_value (header_t h, const char *name, char *buffer,
		  size_t buflen, size_t *n)
{
  size_t i = 0;
  size_t name_len;
  size_t total = 0, fn_len = 0, fv_len = 0;
  int threshold;
  rfc822_t rfc;

  if (h == NULL || name == NULL ||
      (rfc = (rfc822_t)h->data) == NULL)
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
  for (name_len = strlen (name), i = 0; i < rfc->hdr_count; i++)
    {
      fn_len = rfc->hdr[i].fn_end - rfc->hdr[i].fn;
      if (fn_len == name_len && memcmp (rfc->hdr[i].fn, name, fn_len) == 0)
	{
	  fv_len = (rfc->hdr[i].fv_end - rfc->hdr[i].fv);
	  total += fv_len;
	  /* can everything fit in the buffer */
	  if (buffer && threshold > 0)
	    {
	      threshold -= fv_len;
	      if (threshold > 0)
		{
		  memcpy (buffer, rfc->hdr[i].fv, fv_len);
		  buffer += fv_len;
		}
	      else if (threshold < 0)
		{
		  threshold += fv_len;
		  memcpy (buffer, rfc->hdr[i].fv, threshold);
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

static int
rfc822_entry_count (header_t h, size_t *num)
{
  rfc822_t rfc;
  if (h == NULL || (rfc = (rfc822_t)h->data) == NULL)
      return EINVAL;
  if (num)
    *num = rfc->hdr_count;
  return 0;
}

static int
rfc822_entry_name (header_t h, size_t num, char *buf,
		   size_t buflen, size_t *nwritten)
{
  rfc822_t rfc;
  size_t len;
  if (h == NULL || (rfc = (rfc822_t)h->data) == NULL)
    return EINVAL;
  if (rfc->hdr_count == 0 || num > rfc->hdr_count)
    return ENOENT;
  len = rfc->hdr[num].fn_end - rfc->hdr[num].fn;
  /* save one for the null */
  --buflen;
  if (buf && buflen > 0)
    {
      buflen = (len > buflen) ? buflen : len;
      memcpy (buf, rfc->hdr[num].fn, buflen);
      buf[buflen] = '\0';
    }
  if (nwritten)
    *nwritten = len;
  return 0;
}

static int
rfc822_entry_value (header_t h, size_t num, char *buf,
		    size_t buflen, size_t *nwritten)
{
  rfc822_t rfc;
  size_t len;
  if (h == NULL || (rfc = (rfc822_t)h->data) == NULL)
    return EINVAL;
  if (rfc->hdr_count == 0 || num > rfc->hdr_count)
    return ENOENT;
  len = rfc->hdr[num].fv_end - rfc->hdr[num].fv;
  /* save one for the null */
  --buflen;
  if (buf && buflen > 0)
    {
      buflen = (len > buflen) ? buflen : len;
      memcpy (buf, rfc->hdr[num].fv, buflen);
      buf[buflen] = '\0';
    }
  if (nwritten)
    *nwritten = len;
  return 0;
}

static ssize_t
rfc822_get_data (header_t h, char *buf, size_t buflen, off_t off, int *err)
{
  rfc822_t rfc;
  ssize_t len;

  if (h == NULL || (rfc = (rfc822_t)h->data) == NULL)
    {
      if (err)
	*err = EINVAL;
      return -1;
    }

  len = rfc->blurb_len - off;
  if ((rfc->blurb_len - off) > 0)
    {
      if (buf)
	{
	  len = (buflen < (size_t)len) ? buflen : len;
	  memcpy (buf, rfc->blurb + off, len);
	}
    }
  else
    len = 0;

  return len;
}
