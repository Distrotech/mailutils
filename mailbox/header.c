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
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static int rfc822_parse (header_t h, const char *blurb, size_t len);
static int rfc822_set_value (header_t h, const char *fn, const char *fb,
			     size_t n, int replace);
static int rfc822_get_value (header_t h, const char *fn,
			     char *fb, size_t len, size_t *n);
static int rfc822_get_mvalue (header_t h, const char *fn,
			      char **fb, size_t *n);

struct _hdr
{
  char *fn;
  char *fb;
};
typedef struct _hdr *hdr_t;

struct _rfc822_data
{
  char *blurb;
  struct _hdr unixfrom; /* stupid hack for unix from */
  size_t hdr_count;
  struct _hdr *hdr;
};
typedef struct _rfc822_data *rfc822_t;


int
header_init (header_t *ph, const char *blurb, size_t len, int flag)
{
  header_t h;
  if (flag != MU_HDR_RFC822)
    {
      return ENOTSUP;
    }
  h = calloc (1, sizeof (*h));
  if (h == NULL)
    {
      return ENOMEM;
    }
  h->_parse = rfc822_parse;
  h->_get_value = rfc822_get_value;
  h->_get_mvalue = rfc822_get_mvalue;
  h->_set_value = rfc822_set_value;

  h->_parse (h, blurb, len);
  *ph = h;
  return 0;
}

void
header_destroy (header_t *ph)
{
  if (ph && *ph)
    {
      header_t h = *ph;
      if (h->data)
	{
	  rfc822_t rfc = (rfc822_t)h->data;
	  if (rfc->hdr)
	    free (rfc->hdr);
	  if (rfc->blurb)
	    free (rfc->blurb);
	  free (rfc);
	}
      free (h);
      *ph = NULL;
    }
}

int
header_gvalue (const char *blurb, size_t blurb_len, const char *fn, char *fb,
	       size_t len, size_t *n)
{
  const char *header_end;
  const char *header_start;
  const char *colon;
  size_t i = 0;
  size_t j = 0;
  size_t fn_len;
  int status = EINVAL;

  if (blurb == NULL || blurb_len < 5 || fn == NULL || len == 0)
    {
      return status;
    }

  *fb = '\0'; /* if we did find something, return empty */
  for (fn_len = strlen (fn), header_start = blurb;;header_start = header_end)
    {
      header_end = memchr (header_start, '\n', blurb_len);
      if (header_end == NULL)
	{
	  break;
	}
      header_end++;
      blurb_len -= header_end - header_start;
      colon = memchr (header_start, ':', header_end - header_start);
      if (colon == NULL)
	{
	  continue;
	}
      /* check if is the header field name */
      j = colon - header_start;
      if (fn_len == j)
	{
	  if (memcmp (fn, header_start, fn_len) == 0)
	    {
	      i = header_end - colon -1;
	      if (fb)
		{
		  i = (len < i) ? len : i;
		  memcpy (fb, colon +1, i);
		  fb[i - 1] = '\0';
		}
	      status = 0;
	      break;
	    }
	}
    }

  if (n)
    *n = i;
  return status;
}

int
header_gmvalue (const char *blurb, size_t blurb_ln, const char *fn,
		char **fb, size_t *n)
{
  size_t i;
  if (blurb == NULL || blurb_ln == 0 || fn == NULL || fb == NULL
      || header_gvalue (blurb, blurb_ln, fn, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *fb = calloc (1, ++i);
  if (*fb == NULL)
    return ENOMEM;
  return header_gvalue (blurb, blurb_ln, fn, *fb, i, n);
}

static int
rfc822_parse (header_t h, const char *blurb, size_t len)
{
  rfc822_t rfc;
  char *header_end;
  char *header_start;
  char *header_start2;
  char *colon;
  struct _hdr *hdr;

  if (h == NULL || blurb == NULL || len == 0)
    {
      return EINVAL;
    }

  rfc = calloc (1, sizeof (*rfc));
  if (rfc == NULL)
    {
      return ENOMEM;
    }

  rfc->blurb = calloc (1, len);
  if (rfc->blurb == NULL)
    {
      free (rfc);
      return ENOMEM;
    }
  memcpy (rfc->blurb, blurb, len);

  for (header_start = rfc->blurb;; header_start = ++header_end)
    {
      /* get a header, a header is :
	 field-name ':' field-body1
	 [ ' ' '\t' field-body2 ] '\r' '\n'
      */
      for (header_start2 = header_start;;header_start2 = ++header_end)
	{
	  header_end = memchr (header_start2, '\n', len);
	  if (header_end == NULL)
	    {
	      break;
	    }
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
		{
		  break; /* new header break the inner for */
		}
	    }
	  /* *header_end = ' ';  smash LF */
	}

      if (header_end == NULL)
	{
	  break; /* bail out */
	}
      *header_end = '\0'; /* smash LF */

      /* Treats unix "From " specially */
      if ((header_end - header_start >= 5)
	  && strncmp (header_start, "From ", 5) == 0)
	{
	  rfc->unixfrom.fn = "From ";
	  rfc->unixfrom.fb = header_start + 6;
	}
      else
	{
	  colon = memchr (header_start, ':', header_end - header_start);
	  if (colon == NULL)
	    {
	      /* Houston we have a problem */
	      free (rfc->blurb);
	      if (rfc->hdr)
		free (rfc->hdr);
	      free (rfc);
	      return EINVAL;
	    }
	  hdr = realloc (rfc->hdr, (rfc->hdr_count + 1) * sizeof (*hdr));
	  if (hdr == NULL)
	    {
	      /* Houston we have another problem */
	      free (rfc->blurb);
	      if (rfc->hdr)
		free (rfc->hdr);
	      free (rfc);
	      return ENOMEM;
	    }
	  rfc->hdr = hdr;
	  rfc->hdr_count++;
	  *colon = '\0'; /* smash colon */
	  rfc->hdr[rfc->hdr_count - 1].fn = header_start;
	  rfc->hdr[rfc->hdr_count - 1].fb = colon + 1;
	}
    }
  h->data = rfc;
  return 0;
}

static int
rfc822_set_value (header_t h, const char *fn, const char *fb,
		  size_t n, int replace)
{
  return ENOSYS;
}

static int
rfc822_get_value (header_t h, const char *fn, char *fb, size_t len, size_t *n)
{
  size_t i = 0;
  size_t j = 0;
  rfc822_t rfc = (rfc822_t)h->data;

  if (fb)
    {
      *fb = '\0'; /* if empty */
    }
  for (i = 0; i < rfc->hdr_count; i++)
    {
      if (strcmp (rfc->hdr[i].fn, fn) == 0)
	{
	  j = strlen (rfc->hdr[i].fb);
	  j = (len < j) ? len : j;
	  if (fb)
	    {
	      memcpy (fb, rfc->hdr[i].fb, j);
	    }
	  break;
	}
    }
  if (n)
    *n = j;
  return 0;
}

static int
rfc822_get_mvalue (header_t h, const char *fn, char **fb, size_t *n)
{
  size_t i;
  if (h == NULL || fn == NULL || fb == NULL
      || h->_get_value (h, fn, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *fb = calloc (1, ++i);
  if (*fb == NULL)
    return ENOMEM;
  return h->_get_value (h, fn, *fb, i, n);
}
