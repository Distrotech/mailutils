/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>

struct _mu_linelen_filter
{
  size_t max_len;
  size_t cur_len;
};

static enum mu_filter_result
_ll_encoder (void *xd,
	     enum mu_filter_command cmd,
	     struct mu_filter_io *iobuf)
{
  struct _mu_linelen_filter *flt = xd;
  const char *iptr;
  size_t isize;
  char *optr;
  size_t osize;
  size_t consumed, written;
  
  switch (cmd)
    {
    case mu_filter_init:
      flt->cur_len = 0;
    case mu_filter_done:
      return mu_filter_ok;
    default:
      break;
    }

  iptr = iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (consumed = written = 0; consumed < isize && written < osize; )
    {
      char *p;
      size_t rest = flt->max_len - flt->cur_len;
      size_t len = isize - consumed;

      if (len > rest)
	len = rest;
      
      p = memchr (iptr + consumed, '\n', len);
      if (p)
	len = p - iptr - consumed + 1;
      
      rest = osize - written;
      if (len > rest)
	{
	  len = rest;
	  p = NULL;
	}
      memcpy (optr + written, iptr + consumed, len);      
      written += len;
      consumed += len;
      if (p)
	flt->cur_len = 0;
      else
	{
	  flt->cur_len += len;
	  if (flt->cur_len == flt->max_len)
	    {
	      if (written < osize)
		optr[written++] = '\n';
	      flt->cur_len = 0;
	    }
	}
    }
  iobuf->isize = consumed;
  iobuf->osize = written;
  return mu_filter_ok;
}

int
mu_linelen_filter_create (mu_stream_t *pstream, mu_stream_t stream,
			  size_t limit,
			  int flags)
{
  struct _mu_linelen_filter *flt = calloc (1, sizeof (*flt));
  if (!flt)
    return ENOMEM;
  flt->max_len = limit;
  return mu_filter_stream_create (pstream, stream,
				  MU_FILTER_ENCODE, _ll_encoder, flt, flags);
}

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  struct _mu_linelen_filter *flt = malloc (sizeof (flt[0]));
  
  if (!flt)
    return ENOMEM;
  flt->cur_len = 0;
  flt->max_len = 76;
  if (argc > 1)
    {
      char *p;
      flt->max_len = strtoul (argv[1], &p, 10);
      if (*p)
	{
	  free (flt);
	  return MU_ERR_PARSE;
	}
    }
  *pret = flt;
  return 0;
}

static struct _mu_filter_record _linelen_filter = {
  "LINELEN",
  alloc_state,
  _ll_encoder,
  NULL,
};

mu_filter_record_t mu_linelen_filter = &_linelen_filter;
