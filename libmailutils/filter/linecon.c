/* Simple inline comment filter for GNU Mailutils.
   Copyright (C) 2010 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

/* linecon - a line continuation filter.

   This filter has only decode mode.  It removes from the input
   backslashes immediately followed by a newline, thus implementing
   a familiar UNIX line-continuation facility. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/cctype.h>

static enum mu_filter_result
_linecon_decoder (void *xd, enum mu_filter_command cmd,
		  struct mu_filter_io *iobuf)
{
  int *escaped = xd;
  const unsigned char *iptr, *iend;
  char *optr, *oend;

  switch (cmd)
    {
    case mu_filter_init:
      *escaped = 0;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  iend = iptr + iobuf->isize;
  optr = iobuf->output;
  oend = optr + iobuf->osize;

  while (iptr < iend && optr < oend)
    {
      int c = *iptr++;
      switch (c)
	{
	case '\\':
	  *escaped = 1;
	  continue;

	case '\n':
	  if (*escaped)
	    {
	      *escaped = 0;
	      continue;
	    }
	  *optr++ = c;
	  break;

	default:
	  if (*escaped)
	    {
	      *escaped = 0;
	      *optr++ = '\\';
	      if (optr == oend)
		{
		  iptr--;
		  break;
		}
	    }
	  *optr++ = c;
	}
    }

  iobuf->isize = iptr - (const unsigned char *) iobuf->input;
  iobuf->osize = optr - iobuf->output;
  return mu_filter_ok;
}

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  *pret = malloc (sizeof (int));
  if (!*pret)
    return ENOMEM;
  return 0;
}

static struct _mu_filter_record _linecon_filter = {
  "LINECON",
  0,
  alloc_state,
  NULL,
  _linecon_decoder,
};

mu_filter_record_t mu_linecon_filter = &_linecon_filter;
