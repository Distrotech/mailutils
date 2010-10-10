/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2009,
   2010 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/cstr.h>

enum mu_iconv_fallback_mode mu_default_fallback_mode = mu_fallback_copy_octal;

int
mu_set_default_fallback (const char *str)
{
  if (strcmp (str, "none") == 0)
    mu_default_fallback_mode = mu_fallback_none;
  else if (strcmp (str, "copy-pass") == 0)
    mu_default_fallback_mode = mu_fallback_copy_pass;
  else if (strcmp (str, "copy-octal") == 0)
    mu_default_fallback_mode = mu_fallback_copy_octal;
  else
    return EINVAL;
  return 0;
}

int
mu_decode_filter (mu_stream_t *pfilter, mu_stream_t input,
		  const char *filter_type,
		  const char *fromcode, const char *tocode)
{
  mu_stream_t filter;
  
  int status = mu_filter_create (&filter, input, filter_type,
				 MU_FILTER_DECODE, MU_STREAM_READ);
  if (status)
    return status;

  if (fromcode && tocode && mu_c_strcasecmp (fromcode, tocode))
    {
      mu_stream_t cvt;

      status = mu_filter_iconv_create (&cvt, filter, fromcode, tocode,
				       0, mu_default_fallback_mode);
      if (status == 0)
	{
          mu_stream_unref (filter);
          filter = cvt;
	}
    }
  *pfilter = filter;
  return 0;
}

