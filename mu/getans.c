/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/mailutils.h>
#include "mu.h"

int
mu_vgetans (const char *variants, const char *fmt, va_list ap)
{
  char repl[64];

  while (1)
    {
      size_t n;
      char *p;
      
      mu_stream_vprintf (mu_strout, fmt, ap);
      mu_stream_write (mu_strout, "? ", 2, NULL);
      mu_stream_flush (mu_strout);
      if (mu_stream_read (mu_strin, repl, sizeof repl, &n) || n == 0)
	return 0;
      mu_rtrim_class (repl, MU_CTYPE_ENDLN);

      p = strchr (variants, *repl);
      if (p)
	return *p;
      
      mu_stream_printf (mu_strout, _("Please answer one of [%s]: "), variants);
    }
  return 0; /* to pacify gcc */
}

int
mu_getans (const char *variants, const char *fmt, ...)
{
  va_list ap;
  int rc;
  
  va_start (ap, fmt);
  rc = mu_vgetans (variants, fmt, ap);
  va_end (ap);
  return rc;
}

