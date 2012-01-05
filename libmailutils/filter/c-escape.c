/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

/* A C-escape filter.
   In encode mode, replaces control characters with their C escapes.
   For example, newline (ASCII 10) becomes \n, horizontal tab (ASCII 9)
   becomes \t etc.
   In decode mode, the reverse operation is performed, i.e. each C escape
   is replaced with the corresponding character.  Unrecognized escape
   sequences are copied verbatim.

   FIXME: Only named escapes are supported.
*/
   
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/wordsplit.h>
#include <mailutils/cctype.h>

/* Move min(isize,osize) bytes from iptr to optr, replacing each
   escapable control character with its escape sequence. */
static enum mu_filter_result
_c_escape_encoder (void *xd MU_ARG_UNUSED,
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

      if (mu_iscntrl (c))
	{
	  int x;

	  x = mu_wordsplit_c_quote_char (c);
	  if (x == -1)
	    optr[j++] = c;
	  else if (j + 1 == osize)
	    {
	      if (i == 0)
		{
		  iobuf->osize = 2;
		  return mu_filter_moreoutput;
		}
	      break;
	    }
	  else
	    {
	      optr[j++] = '\\';
	      optr[j++] = x;
	    }
	}
      else
	optr[j++] = c;
    }
  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

/* Move min(isize,osize) bytes from iptr to optr, replacing each escape
   sequence with its ASCII code. */
static enum mu_filter_result
_c_escape_decoder (void *xd MU_ARG_UNUSED,
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
      if (c == '\\')
	{
	  int x;
	  if (i + 1 == isize)
	    break;
	  c = *iptr++;
	  i++;
	  x = mu_wordsplit_c_unquote_char (c);
	  optr[j++] = (x == -1) ? c : x;
	}
      else
	optr[j++] = c;
    }

  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

static struct _mu_filter_record _c_escape_filter = {
  "C-escape",
  NULL,
  _c_escape_encoder,
  _c_escape_decoder,
};

mu_filter_record_t mu_c_escape_filter = &_c_escape_filter;

