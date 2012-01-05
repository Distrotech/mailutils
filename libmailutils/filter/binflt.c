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

static enum mu_filter_result
_copy_codec (void *xd MU_ARG_UNUSED,
	     enum mu_filter_command cmd,
	     struct mu_filter_io *iobuf)
{
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
    case mu_filter_done:
      return mu_filter_ok;
    default:
      break;
    }
  
  osize = iobuf->osize;
  if (osize > iobuf->isize)
    osize = iobuf->isize;
  memcpy (iobuf->output, iobuf->input, osize);
  iobuf->isize = osize;
  iobuf->osize = osize;
  return mu_filter_ok;
}

static enum mu_filter_result
_bit7_coder (void *xd MU_ARG_UNUSED,
	     enum mu_filter_command cmd,
	     struct mu_filter_io *iobuf)
{
  size_t i;
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

  if (osize > isize)
    osize = isize;
  for (i = 0; i < osize; i++)
    *optr++ = *iptr++ & 0x7f;
  iobuf->isize = osize;
  iobuf->osize = osize;
  return mu_filter_ok;
}

static struct _mu_filter_record _binary_filter = {
  "binary",
  NULL,
  _copy_codec,
  _copy_codec
};

mu_filter_record_t mu_binary_filter = &_binary_filter;


static struct _mu_filter_record _bit8_filter = {
  "8bit",
  NULL,
  _copy_codec,
  _copy_codec
};

mu_filter_record_t mu_bit8_filter = &_bit8_filter;

static struct _mu_filter_record _bit7_filter = {
  "7bit",
  NULL,
  _bit7_coder,
  _copy_codec
};

mu_filter_record_t mu_bit7_filter = &_bit7_filter;
