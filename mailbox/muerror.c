/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <mailutils/error.h>

int
mu_default_error_printer (const char *fmt, va_list ap)
{
  int status;
  status = vfprintf (stderr, fmt, ap);
  if (status >= 0)
    {
      if (fputc ('\n', stderr) != EOF)
	status++;
    }
  return status;
}

int
mu_syslog_error_printer (const char *fmt, va_list ap)
{
#ifdef HAVE_VSYSLOG
  vsyslog (LOG_CRIT, fmt, ap);
#else
  char buf[128];
  vsnprintf (buf, sizeof buf, fmt, ap);
  syslog (LOG_CRIT, "%s", buf);
#endif
  return 0;
}

static mu_error_pfn_t mu_error_printer = mu_default_error_printer;

int
mu_verror (const char *fmt, va_list ap)
{
  if (mu_error_printer)
    return (*mu_error_printer) (fmt, ap);
  return 0;
}

int
mu_error (const char *fmt, ...)
{
  int status;
  va_list ap;

  if (!mu_error_printer)
    return 0;
  va_start (ap, fmt);
  status = (*mu_error_printer) (fmt, ap);
  va_end (ap);
  return status;
}

void
mu_error_set_print (mu_error_pfn_t pfn)
{
  mu_error_printer = pfn;
}
