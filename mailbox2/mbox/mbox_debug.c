/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
# include <config.h>
#endif

#include <string.h>

#include <stdio.h>
#include <stdarg.h>
#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_get_debug (mbox_t mbox, mu_debug_t *pdebug)
{
  if (mbox == NULL || pdebug == NULL)
    return MU_ERROR_INVALID_PARAMETER;


  if (mbox->debug == NULL)
    {
      stream_t stream;
      int status = stream_stdio_create (&stream, stderr);
      if (status == 0)
	status = mu_debug_stream_create (&mbox->debug, stream, 0);
      if (status != 0)
	return status;
    }
  mu_debug_ref (mbox->debug);
  *pdebug = mbox->debug;
  return 0;
}

int
mbox_set_debug (mbox_t mbox, mu_debug_t debug)
{
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (mbox->debug)
    mu_debug_destroy (&mbox->debug);
  mbox->debug = debug;
  return 0;
}

int
mbox_debug_print (mbox_t mbox, const char *fmt, ...)
{
  if (mbox && mbox->debug)
    {
      char buf[128];
      va_list ap;
      snprintf (buf, sizeof buf, "mbox(%s)_",
		(mbox->filename) ? mbox->filename : "");
      mu_debug_print (mbox->debug, MU_DEBUG_TRACE, buf);
      if (fmt)
	{
	  va_start (ap, fmt);
	  vsnprintf (buf, sizeof buf, fmt, ap);
	  va_end (ap);
	}
      mu_debug_print (mbox->debug, MU_DEBUG_TRACE, buf);
      mu_debug_print (mbox->debug, MU_DEBUG_TRACE, "\n");
    }
  return 0;
}
