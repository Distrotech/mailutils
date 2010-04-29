/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

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

/* Move min(isize,osize) bytes from iptr to optr, replacing each \n
   with \r\n.  Store number of bytes written to optr in *pnbytes.
   Return number of bytes read from iptr. */
static enum mu_filter_result
_crlf_encoder (void *xd MU_ARG_UNUSED,
	       enum mu_filter_command cmd,
	       struct mu_filter_io *iobuf)
{
  size_t i, j;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
    case mu_filter_done:
      return mu_filter_ok;
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (i = j = 0; i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;
      if (c == '\n')
	{
	  if (j + 1 == osize)
	    {
	      if (i == 0)
		{
		  iobuf->osize = 2;
		  return mu_filter_moreoutput;
		}
	      break;
	    }
	  optr[j++] = '\r';
	  optr[j++] = '\n';
	}
      /*else if (c == '\r')*/
      else
	optr[j++] = c;
    }
  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

/* Move min(isize,osize) bytes from iptr to optr, replacing each \r\n
   with \n.  Store number of bytes written to optr in *pnbytes.
   Return number of bytes read from iptr. */
static enum mu_filter_result
_crlf_decoder (void *xd MU_ARG_UNUSED,
	       enum mu_filter_command cmd,
	       struct mu_filter_io *iobuf)
{
  size_t i, j;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
    case mu_filter_done:
      return mu_filter_ok;
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (i = j = 0; i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;
      if (c == '\r')
	{
	  if (i + 1 == isize)
	    break;
	  if (*iptr == '\n')
	    continue;
	}
      optr[j++] = c;
    }
  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

static struct _mu_filter_record _crlf_filter = {
  "CRLF",
  0,
  NULL,
  _crlf_encoder,
  _crlf_decoder
};

mu_filter_record_t mu_crlf_filter = &_crlf_filter;


/* For compatibility with previous versions */
static struct _mu_filter_record _rfc822_filter = {
  "RFC822",
  0,
  NULL,
  _crlf_encoder,
  _crlf_decoder
};

mu_filter_record_t mu_rfc822_filter = &_rfc822_filter;


