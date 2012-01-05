/* Message header filter for GNU Mailutils.
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

/* This filter reads the input data up to the first empty line (i.e. \n\n).
   It is suitable to extract headers from RFC822 messages.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/cctype.h>

enum hflt_state
  {
    hflt_initial,
    hflt_newline,
    hflt_copy
  };

static int
new_state (int *pd, unsigned c)
{
  switch (*pd)
    {
    case hflt_initial:
    case hflt_copy:
      *pd = (c == '\n') ? hflt_newline : hflt_copy;
      break;
      
    case hflt_newline:
      if (c == '\n')
	return 1;
      *pd = hflt_copy;
      break;
    }
  return 0;
}

static enum mu_filter_result
_hflt_decoder (void *xd, enum mu_filter_command cmd,
		struct mu_filter_io *iobuf)
{
  int *pd = xd;
  size_t i, j;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
      *pd = hflt_initial;
      return mu_filter_ok;
      
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
      if (new_state (pd, c))
	{
	  iobuf->eof = 1;
	  break;
	}
      optr[j++] = c;
    }

  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}
  
static int
alloc_state (void **pret, int mode MU_ARG_UNUSED,
	     int argc MU_ARG_UNUSED, const char **argv MU_ARG_UNUSED)
{
  int *pd = malloc (sizeof (*pd));
  
  if (!pd)
    return ENOMEM;
  *pret = pd;
  return 0;
}

static struct _mu_filter_record _header_filter = {
  "HEADER",
  alloc_state,
  NULL,
  _hflt_decoder
};

mu_filter_record_t mu_header_filter = &_header_filter;
